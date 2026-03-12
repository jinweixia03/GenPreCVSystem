#!/usr/bin/env python3
"""
深度学习推理服务

这是一个常驻内存的 Python 服务，通过 stdin/stdout 与 C++ 应用通信。
使用 JSON 格式进行请求和响应。

基于 base_service.py 提供的基类实现。

使用方法:
    python dl_service.py

通信协议:
    请求 (stdin): JSON 格式
    响应 (stdout): JSON 格式

请求格式:
    {
        "command": "load_model",
        "model_path": "path/to/model.pt",
        "labels_path": "path/to/labels.txt"  // 可选
    }

    {
        "command": "detect",
        "image_path": "path/to/image.jpg",
        "conf_threshold": 0.25,
        "iou_threshold": 0.45,
        "image_size": 640
    }

    {
        "command": "exit"
    }

响应格式:
    {
        "success": true/false,
        "message": "状态消息",
        "data": { ... }  // 具体数据
    }
"""

import sys
import json
import os
from pathlib import Path
from typing import Dict, Any

# 导入基类
from base_service import BaseService, ModelServiceMixin, ImageServiceMixin

# 尝试导入 ultralytics
try:
    from ultralytics import YOLO
    HAS_ULTRALYTICS = True
except ImportError:
    HAS_ULTRALYTICS = False

# 尝试导入 torch
try:
    import torch
    HAS_TORCH = True
except ImportError:
    HAS_TORCH = False

# 尝试导入 torchvision NMS
try:
    from torchvision.ops import nms as torchvision_nms
    HAS_TORCHVISION_NMS = True
except ImportError:
    HAS_TORCHVISION_NMS = False

# 尝试导入 OpenCV NMS
try:
    import cv2
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False


class DLService(BaseService, ModelServiceMixin, ImageServiceMixin):
    """深度学习推理服务"""

    # 默认参数常量
    DEFAULT_CONF_THRESHOLD = 0.25
    DEFAULT_IOU_THRESHOLD = 0.45
    DEFAULT_IMAGE_SIZE = 640
    DEFAULT_TOP_K = 5

    # 参数范围限制
    MIN_CONF_THRESHOLD = 0.01
    MAX_CONF_THRESHOLD = 1.0
    MIN_IOU_THRESHOLD = 0.01
    MAX_IOU_THRESHOLD = 1.0
    MIN_IMAGE_SIZE = 32
    MAX_IMAGE_SIZE = 4096
    MAX_TOP_K = 1000

    def __init__(self):
        BaseService.__init__(self, "DL")
        ModelServiceMixin.__init__(self)
        ImageServiceMixin.__init__(self)

    def get_service_info(self) -> Dict[str, Any]:
        """获取服务信息"""
        device = "unknown"
        if HAS_TORCH:
            device = "cuda" if torch.cuda.is_available() else "cpu"

        return {
            "has_ultralytics": HAS_ULTRALYTICS,
            "has_torch": HAS_TORCH,
            "has_torchvision_nms": HAS_TORCHVISION_NMS,
            "has_cv2": HAS_CV2,
            "device": device,
            "default_conf": self.DEFAULT_CONF_THRESHOLD,
            "default_iou": self.DEFAULT_IOU_THRESHOLD,
            "default_image_size": self.DEFAULT_IMAGE_SIZE
        }

    def apply_nms(self, detections: list, iou_threshold: float) -> list:
        """
        应用非极大值抑制 (NMS) 后处理

        作为保险措施：即使模型不是标准 YOLO 格式或内部 NMS 失效，
        也能确保正确应用 IOU 阈值去除重叠框。

        Args:
            detections: 检测结果列表，每个元素为 dict(x, y, width, height, confidence, class_id, ...)
            iou_threshold: IOU 阈值

        Returns:
            经过 NMS 过滤后的检测结果列表
        """
        if not detections or len(detections) <= 1:
            return detections

        # 如果 torchvision NMS 可用，优先使用
        if HAS_TORCHVISION_NMS and HAS_TORCH:
            try:
                # 转换为 tensor 格式 [x1, y1, x2, y2, score]
                boxes = []
                scores = []
                for det in detections:
                    x1 = det["x"]
                    y1 = det["y"]
                    x2 = det["x"] + det["width"]
                    y2 = det["y"] + det["height"]
                    boxes.append([x1, y1, x2, y2])
                    scores.append(det["confidence"])

                boxes_tensor = torch.tensor(boxes, dtype=torch.float32)
                scores_tensor = torch.tensor(scores, dtype=torch.float32)

                # 应用 NMS
                keep_indices = torchvision_nms(boxes_tensor, scores_tensor, iou_threshold)

                # 返回保留的检测结果
                return [detections[i] for i in keep_indices.tolist()]
            except Exception as e:
                print(f"警告: torchvision NMS 失败: {e}，将使用备选方案", file=sys.stderr)

        # 如果 OpenCV NMS 可用，使用 OpenCV 实现
        if HAS_CV2:
            try:
                boxes = []
                scores = []
                for det in detections:
                    x = det["x"]
                    y = det["y"]
                    w = det["width"]
                    h = det["height"]
                    boxes.append([x, y, w, h])
                    scores.append(det["confidence"])

                # OpenCV NMS
                indices = cv2.dnn.NMSBoxes(boxes, scores, score_threshold=0.0, nms_threshold=iou_threshold)

                if len(indices) > 0:
                    if isinstance(indices, tuple):
                        indices = indices[0]
                    indices = indices.flatten().tolist()
                    return [detections[i] for i in indices]
                return []
            except Exception as e:
                print(f"警告: OpenCV NMS 失败: {e}", file=sys.stderr)

        # 如果都不可用，返回原始结果（依赖模型内部 NMS）
        return detections

    def handle_command(self, command: str, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理命令"""
        if not HAS_ULTRALYTICS:
            return self.create_error_response("ultralytics 库未安装。请运行: pip install ultralytics")

        handlers = {
            "load_model": self._handle_load_model,
            "detect": self._handle_detect,
            "segment": self._handle_segment,
            "classify": self._handle_classify,
            "keypoint": self._handle_keypoint,
        }

        handler = handlers.get(command)
        if handler:
            return handler(request)
        else:
            return self.create_error_response(f"未知命令: {command}")

    def _handle_load_model(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理加载模型命令"""
        model_path = request.get("model_path", "")
        labels_path = request.get("labels_path")

        # 验证模型路径
        valid, error_msg = self.validate_model_path(model_path)
        if not valid:
            return self.create_error_response(error_msg)

        try:
            # 加载模型
            self.model = YOLO(model_path)
            self.model_path = model_path

            # 加载类别标签
            self.class_names = self.load_class_names(labels_path)
            if not self.class_names and hasattr(self.model, 'names'):
                self.class_names = list(self.model.names.values())

            self.model_loaded = True

            return self.create_success_response(
                message=f"模型加载成功: {model_path}",
                data={
                    "num_classes": len(self.class_names),
                    "class_names": self.class_names[:10]  # 只返回前10个
                }
            )

        except Exception as e:
            self.model_loaded = False
            import traceback
            return self.create_error_response(f"加载模型失败: {str(e)}", traceback.format_exc())

    def _get_validated_image_params(self, request: Dict[str, Any]) -> tuple:
        """获取并验证图像处理参数"""
        image_path = request.get("image_path", "")

        # 验证图像路径
        valid, error_msg = self.validate_image_path(image_path)
        if not valid:
            return None, self.create_error_response(error_msg)

        if not self.model_loaded:
            return None, self.create_error_response("模型未加载")

        # 获取并验证参数
        conf_threshold = float(request.get("conf_threshold", self.DEFAULT_CONF_THRESHOLD))
        iou_threshold = float(request.get("iou_threshold", self.DEFAULT_IOU_THRESHOLD))
        image_size = int(request.get("image_size", self.DEFAULT_IMAGE_SIZE))

        # 限制参数范围
        conf_threshold = max(self.MIN_CONF_THRESHOLD, min(self.MAX_CONF_THRESHOLD, conf_threshold))
        iou_threshold = max(self.MIN_IOU_THRESHOLD, min(self.MAX_IOU_THRESHOLD, iou_threshold))
        image_size = max(self.MIN_IMAGE_SIZE, min(self.MAX_IMAGE_SIZE, image_size))

        return (image_path, conf_threshold, iou_threshold, image_size), None

    def _handle_detect(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理目标检测命令"""
        params, error = self._get_validated_image_params(request)
        if error:
            return error

        image_path, conf_threshold, iou_threshold, image_size = params

        try:
            # 执行推理
            results = self.model(
                image_path,
                conf=conf_threshold,
                iou=iou_threshold,
                imgsz=image_size,
                verbose=False
            )

            # 解析结果
            detections = []
            for result in results:
                boxes = result.boxes
                if boxes is not None:
                    for i in range(len(boxes)):
                        box = boxes.xyxy[i].cpu().numpy()
                        conf = float(boxes.conf[i].cpu().numpy())
                        cls_id = int(boxes.cls[i].cpu().numpy())

                        x1, y1, x2, y2 = box
                        detections.append({
                            "x": int(x1),
                            "y": int(y1),
                            "width": int(x2 - x1),
                            "height": int(y2 - y1),
                            "confidence": round(conf, 4),
                            "class_id": cls_id,
                            "label": self.class_names[cls_id] if cls_id < len(self.class_names) else f"class_{cls_id}"
                        })

            # 应用 NMS 后处理（保险措施）
            original_count = len(detections)
            detections = self.apply_nms(detections, iou_threshold)
            filtered_count = len(detections)

            # 记录 NMS 效果
            if filtered_count < original_count:
                print(f"NMS 过滤: {original_count} -> {filtered_count} (IOU={iou_threshold})", file=sys.stderr)

            return self.create_success_response(
                message=f"检测完成，发现 {len(detections)} 个目标",
                data={
                    "detections": detections,
                    "count": len(detections),
                    "nms_applied": filtered_count < original_count,
                    "nms_filtered": original_count - filtered_count
                }
            )

        except Exception as e:
            import traceback
            return self.create_error_response(f"检测失败: {str(e)}", traceback.format_exc())

    def _handle_segment(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理实例分割命令"""
        params, error = self._get_validated_image_params(request)
        if error:
            return error

        image_path, conf_threshold, iou_threshold, image_size = params

        try:
            # 执行推理
            results = self.model(
                image_path,
                conf=conf_threshold,
                iou=iou_threshold,
                imgsz=image_size,
                verbose=False
            )

            # 解析结果
            instances = []
            for result in results:
                boxes = result.boxes
                masks = result.masks

                if boxes is not None:
                    for i in range(len(boxes)):
                        box = boxes.xyxy[i].cpu().numpy()
                        conf = float(boxes.conf[i].cpu().numpy())
                        cls_id = int(boxes.cls[i].cpu().numpy())

                        x1, y1, x2, y2 = box

                        instance = {
                            "x": int(x1),
                            "y": int(y1),
                            "width": int(x2 - x1),
                            "height": int(y2 - y1),
                            "confidence": round(conf, 4),
                            "class_id": cls_id,
                            "label": self.class_names[cls_id] if cls_id < len(self.class_names) else f"class_{cls_id}"
                        }

                        # 提取掩码多边形
                        mask_polygon = []
                        if masks is not None and i < len(masks):
                            if hasattr(masks, 'xy') and masks.xy is not None:
                                polygon = masks.xy[i]
                                if polygon is not None and len(polygon) > 0:
                                    for pt in polygon:
                                        mask_polygon.append({
                                            "x": float(pt[0]),
                                            "y": float(pt[1])
                                        })

                        instance["mask_polygon"] = mask_polygon
                        instances.append(instance)

            # 应用 NMS 后处理（保险措施）
            original_count = len(instances)
            instances = self.apply_nms(instances, iou_threshold)
            filtered_count = len(instances)

            # 记录 NMS 效果
            if filtered_count < original_count:
                print(f"分割 NMS 过滤: {original_count} -> {filtered_count} (IOU={iou_threshold})", file=sys.stderr)

            return self.create_success_response(
                message=f"分割完成，发现 {len(instances)} 个实例",
                data={
                    "detections": instances,
                    "count": len(instances),
                    "nms_applied": filtered_count < original_count,
                    "nms_filtered": original_count - filtered_count
                }
            )

        except Exception as e:
            import traceback
            return self.create_error_response(f"分割失败: {str(e)}", traceback.format_exc())

    def _handle_classify(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理图像分类命令"""
        image_path = request.get("image_path", "")

        # 验证图像路径
        valid, error_msg = self.validate_image_path(image_path)
        if not valid:
            return self.create_error_response(error_msg)

        if not self.model_loaded:
            return self.create_error_response("模型未加载")

        top_k = int(request.get("top_k", self.DEFAULT_TOP_K))
        top_k = max(1, min(self.MAX_TOP_K, top_k))

        try:
            # 执行推理
            results = self.model(image_path, verbose=False)

            # 解析结果
            classifications = []
            for result in results:
                probs = result.probs
                if probs is not None:
                    values, indices = torch.topk(probs.data, min(top_k, len(probs.data)))
                    for i, (val, idx) in enumerate(zip(values, indices)):
                        classifications.append({
                            "rank": i + 1,
                            "confidence": round(float(val.cpu().numpy()), 4),
                            "class_id": int(idx.cpu().numpy()),
                            "label": self.class_names[int(idx.cpu().numpy())] if int(idx.cpu().numpy()) < len(self.class_names) else f"class_{idx}"
                        })

            return self.create_success_response(
                message=f"分类完成，Top-1: {classifications[0]['label'] if classifications else 'N/A'}",
                data={
                    "classifications": classifications,
                    "top_prediction": classifications[0] if classifications else None
                }
            )

        except Exception as e:
            import traceback
            return self.create_error_response(f"分类失败: {str(e)}", traceback.format_exc())

    def _handle_keypoint(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理关键点检测命令"""
        params, error = self._get_validated_image_params(request)
        if error:
            return error

        image_path, conf_threshold, iou_threshold, image_size = params

        try:
            # 执行推理
            results = self.model(
                image_path,
                conf=conf_threshold,
                iou=iou_threshold,
                imgsz=image_size,
                verbose=False
            )

            # 解析结果
            detections = []
            for result in results:
                boxes = result.boxes
                keypoints = result.keypoints

                if boxes is not None:
                    for i in range(len(boxes)):
                        box = boxes.xyxy[i].cpu().numpy()
                        conf = float(boxes.conf[i].cpu().numpy())
                        cls_id = int(boxes.cls[i].cpu().numpy())

                        x1, y1, x2, y2 = box

                        detection = {
                            "x": int(x1),
                            "y": int(y1),
                            "width": int(x2 - x1),
                            "height": int(y2 - y1),
                            "confidence": round(conf, 4),
                            "class_id": cls_id,
                            "label": self.class_names[cls_id] if cls_id < len(self.class_names) else f"class_{cls_id}",
                            "keypoints": []
                        }

                        # 提取关键点
                        if keypoints is not None and i < len(keypoints):
                            kps = keypoints.xy[i].cpu().numpy()
                            kps_conf = keypoints.conf[i].cpu().numpy() if keypoints.conf is not None else None

                            for j, kp in enumerate(kps):
                                kp_data = {
                                    "id": j,
                                    "x": float(kp[0]),
                                    "y": float(kp[1])
                                }
                                if kps_conf is not None and j < len(kps_conf):
                                    kp_data["confidence"] = round(float(kps_conf[j]), 4)
                                detection["keypoints"].append(kp_data)

                        detections.append(detection)

            # 应用 NMS 后处理（保险措施）
            original_count = len(detections)
            detections = self.apply_nms(detections, iou_threshold)
            filtered_count = len(detections)

            # 记录 NMS 效果
            if filtered_count < original_count:
                print(f"关键点检测 NMS 过滤: {original_count} -> {filtered_count} (IOU={iou_threshold})", file=sys.stderr)

            return self.create_success_response(
                message=f"关键点检测完成，发现 {len(detections)} 个目标",
                data={
                    "detections": detections,
                    "count": len(detections),
                    "nms_applied": filtered_count < original_count,
                    "nms_filtered": original_count - filtered_count
                }
            )

        except Exception as e:
            import traceback
            return self.create_error_response(f"关键点检测失败: {str(e)}", traceback.format_exc())

    def cleanup(self):
        """清理资源"""
        self.model = None
        self.model_loaded = False


def main():
    """主函数"""
    service = DLService()
    service.run()


if __name__ == "__main__":
    main()
