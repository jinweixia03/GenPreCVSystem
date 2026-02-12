#!/usr/bin/env python3
"""
YOLO 推理服务

这是一个常驻内存的 Python 服务，通过 stdin/stdout 与 C++ 应用通信。
使用 JSON 格式进行请求和响应。

使用方法:
    python yolo_service.py

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
        "command": "segment",
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

# 尝试导入 ultralytics
try:
    from ultralytics import YOLO
    HAS_ULTRALYTICS = True
except ImportError:
    HAS_ULTRALYTICS = False
    print(json.dumps({
        "success": False,
        "message": "ultralytics 库未安装。请运行: pip install ultralytics"
    }), flush=True)


class YOLOService:
    """YOLO 推理服务"""

    def __init__(self):
        self.model = None
        self.class_names = []
        self.model_path = None

    def load_model(self, model_path: str, labels_path: str = None) -> dict:
        """加载模型"""
        if not HAS_ULTRALYTICS:
            return {"success": False, "message": "ultralytics 库未安装"}

        try:
            # 检查模型文件是否存在
            if not os.path.exists(model_path):
                return {"success": False, "message": f"模型文件不存在: {model_path}"}

            # 加载模型
            self.model = YOLO(model_path)
            self.model_path = model_path

            # 加载类别标签
            if labels_path and os.path.exists(labels_path):
                with open(labels_path, 'r', encoding='utf-8') as f:
                    self.class_names = [line.strip() for line in f if line.strip()]
            else:
                # 使用模型自带的类别名称
                self.class_names = list(self.model.names.values())

            return {
                "success": True,
                "message": f"模型加载成功: {model_path}",
                "data": {
                    "num_classes": len(self.class_names),
                    "class_names": self.class_names[:10]  # 只返回前10个
                }
            }

        except Exception as e:
            return {"success": False, "message": f"加载模型失败: {str(e)}"}

    def detect(self, image_path: str, conf_threshold: float = 0.25,
               iou_threshold: float = 0.45, image_size: int = 640) -> dict:
        """执行目标检测"""
        if self.model is None:
            return {"success": False, "message": "模型未加载"}

        try:
            # 检查图像文件
            if not os.path.exists(image_path):
                return {"success": False, "message": f"图像文件不存在: {image_path}"}

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
                        box = boxes.xyxy[i].cpu().numpy()  # x1, y1, x2, y2
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

            return {
                "success": True,
                "message": f"检测完成，发现 {len(detections)} 个目标",
                "data": {
                    "detections": detections,
                    "count": len(detections)
                }
            }

        except Exception as e:
            return {"success": False, "message": f"检测失败: {str(e)}"}

    def segment(self, image_path: str, conf_threshold: float = 0.25,
                iou_threshold: float = 0.45, image_size: int = 640) -> dict:
        """执行实例分割"""
        if self.model is None:
            return {"success": False, "message": "模型未加载"}

        try:
            # 检查图像文件
            if not os.path.exists(image_path):
                return {"success": False, "message": f"图像文件不存在: {image_path}"}

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
                            # masks.xy 返回多边形坐标 (N, 2)
                            if hasattr(masks, 'xy') and masks.xy is not None:
                                polygon = masks.xy[i]  # 获取第 i 个掩码的多边形
                                if polygon is not None and len(polygon) > 0:
                                    # 将多边形点转换为列表格式
                                    for pt in polygon:
                                        mask_polygon.append({
                                            "x": float(pt[0]),
                                            "y": float(pt[1])
                                        })

                        instance["mask_polygon"] = mask_polygon
                        instances.append(instance)

            return {
                "success": True,
                "message": f"分割完成，发现 {len(instances)} 个实例",
                "data": {
                    "detections": instances,  # 使用统一字段名
                    "count": len(instances)
                }
            }

        except Exception as e:
            return {"success": False, "message": f"分割失败: {str(e)}"}

    def classify(self, image_path: str, top_k: int = 5) -> dict:
        """执行图像分类"""
        if self.model is None:
            return {"success": False, "message": "模型未加载"}

        try:
            # 检查图像文件
            if not os.path.exists(image_path):
                return {"success": False, "message": f"图像文件不存在: {image_path}"}

            # 执行推理
            results = self.model(image_path, verbose=False)

            # 解析结果
            classifications = []
            for result in results:
                probs = result.probs
                if probs is not None:
                    # 获取 top-k 分类结果
                    import torch
                    values, indices = torch.topk(probs.data, min(top_k, len(probs.data)))
                    for i, (val, idx) in enumerate(zip(values, indices)):
                        classifications.append({
                            "rank": i + 1,
                            "confidence": round(float(val.cpu().numpy()), 4),
                            "class_id": int(idx.cpu().numpy()),
                            "label": self.class_names[int(idx.cpu().numpy())] if int(idx.cpu().numpy()) < len(self.class_names) else f"class_{idx}"
                        })

            return {
                "success": True,
                "message": f"分类完成，Top-1: {classifications[0]['label'] if classifications else 'N/A'}",
                "data": {
                    "classifications": classifications,
                    "top_prediction": classifications[0] if classifications else None,
                    "detections": []  # 统一接口
                }
            }

        except Exception as e:
            return {"success": False, "message": f"分类失败: {str(e)}"}

    def keypoint(self, image_path: str, conf_threshold: float = 0.25,
                 iou_threshold: float = 0.45, image_size: int = 640) -> dict:
        """执行关键点/姿态检测"""
        if self.model is None:
            return {"success": False, "message": "模型未加载"}

        try:
            # 检查图像文件
            if not os.path.exists(image_path):
                return {"success": False, "message": f"图像文件不存在: {image_path}"}

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
                            kps = keypoints.xy[i].cpu().numpy()  # (num_keypoints, 2)
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

            return {
                "success": True,
                "message": f"关键点检测完成，发现 {len(detections)} 个目标",
                "data": {
                    "detections": detections,
                    "count": len(detections)
                }
            }

        except Exception as e:
            return {"success": False, "message": f"关键点检测失败: {str(e)}"}


def main():
    """主循环"""
    service = YOLOService()

    # 输出就绪信号
    print(json.dumps({
        "success": True,
        "message": "YOLO 服务已启动",
        "data": {"has_ultralytics": HAS_ULTRALYTICS}
    }), flush=True)

    # 主循环：从 stdin 读取命令
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue

        try:
            request = json.loads(line)
            command = request.get("command", "")

            if command == "exit":
                print(json.dumps({"success": True, "message": "服务已停止"}), flush=True)
                break

            elif command == "load_model":
                response = service.load_model(
                    request.get("model_path", ""),
                    request.get("labels_path")
                )

            elif command == "detect":
                response = service.detect(
                    request.get("image_path", ""),
                    request.get("conf_threshold", 0.25),
                    request.get("iou_threshold", 0.45),
                    request.get("image_size", 640)
                )

            elif command == "segment":
                response = service.segment(
                    request.get("image_path", ""),
                    request.get("conf_threshold", 0.25),
                    request.get("iou_threshold", 0.45),
                    request.get("image_size", 640)
                )

            elif command == "classify":
                response = service.classify(
                    request.get("image_path", ""),
                    request.get("top_k", 5)
                )

            elif command == "keypoint":
                response = service.keypoint(
                    request.get("image_path", ""),
                    request.get("conf_threshold", 0.25),
                    request.get("iou_threshold", 0.45),
                    request.get("image_size", 640)
                )

            else:
                response = {"success": False, "message": f"未知命令: {command}"}

            print(json.dumps(response, ensure_ascii=False), flush=True)

        except json.JSONDecodeError as e:
            print(json.dumps({
                "success": False,
                "message": f"JSON 解析错误: {str(e)}"
            }), flush=True)

        except Exception as e:
            print(json.dumps({
                "success": False,
                "message": f"处理错误: {str(e)}"
            }), flush=True)


if __name__ == "__main__":
    main()
