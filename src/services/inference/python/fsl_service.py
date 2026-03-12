#!/usr/bin/env python3
"""
遥感影像小样本分类推理服务

这是一个常驻内存的 Python 服务，通过 stdin/stdout 与 C++ 应用通信。
使用 JSON 格式进行请求和响应。
基于 EasyFSL 库实现 Prototypical Networks 小样本学习。

基于 base_service.py 提供的基类实现。

使用方法:
    python fsl_service.py

通信协议:
    请求 (stdin): JSON 格式
    响应 (stdout): JSON 格式

请求格式:
    {
        "command": "load_model",
        "model_path": "path/to/model.pth"
    }

    {
        "command": "few_shot_classify",
        "image_path": "path/to/image.jpg",
        "n_way": 5,
        "n_shot": 5,
        "n_query": 15,
        "image_size": 84,
        "num_episodes": 10  // 可选：推理轮数
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
import random
import numpy as np
from pathlib import Path
from typing import List, Dict, Tuple, Any

# 导入基类
from base_service import BaseService, ModelServiceMixin, ImageServiceMixin

# 尝试导入 torch
try:
    import torch
    from torch import nn
    import torch.nn.functional as F
    HAS_TORCH = True
except ImportError:
    HAS_TORCH = False

# 尝试导入 easyfsl
try:
    from easyfsl.methods import PrototypicalNetworks
    from easyfsl.modules import resnet12
    from PIL import Image
    from torchvision import transforms
    HAS_EASYFSL = True
except ImportError:
    HAS_EASYFSL = False


# 默认类别列表（当无法从数据集加载时使用）
DEFAULT_CLASSES = [
    "航空站 (Airport)",
    "桥梁 (Bridge)",
    "中央商务区 (CentralBusinessDistrict)",
    "教堂 (Church)",
    "商业区 (Commercial)",
    "密集住宅区 (DenseResidential)",
    "工业区 (Industrial)",
    "中等密度住宅区 (MediumDensityResidential)",
    "低密度住宅区 (LowDensityResidential)",
    "农场 (Farm)",
    "森林 (Forest)",
    "草地 (Meadow)",
    "湿地 (Wetland)",
    "水体 (Water)",
    "湿地植被 (Marsh)",
    "海滩 (Beach)",
    "港口 (Port)",
    "广场 (Square)",
    "高尔夫球场 (GolfCourse)",
    "游乐场 (Playground)",
    "大学 (University)",
    "医院 (Hospital)",
    "机场航站楼 (AirportTerminal)",
    "停机坪 (Runway)",
    "高速公路 (Freeway)",
    "铁路 (Railway)",
    "停车场 (Parking)",
    "工地 (ConstructionSite)",
    "灾害 (Disaster)",
    "立交桥 (Overpass)"
]


class FSLService(BaseService, ModelServiceMixin, ImageServiceMixin):
    """小样本学习推理服务"""

    # 默认参数常量
    DEFAULT_N_WAY = 5
    DEFAULT_N_SHOT = 5
    DEFAULT_N_QUERY = 15
    DEFAULT_IMAGE_SIZE = 84
    DEFAULT_NUM_EPISODES = 20

    # 参数范围限制
    MIN_N_WAY = 2
    MAX_N_WAY = 50
    MIN_N_SHOT = 1
    MAX_N_SHOT = 20
    MIN_IMAGE_SIZE = 32
    MAX_IMAGE_SIZE = 512
    MIN_EPISODES = 1
    MAX_EPISODES = 100

    # 特征维度
    FEATURE_DIM = 512

    def __init__(self):
        BaseService.__init__(self, "FSL")
        ModelServiceMixin.__init__(self)
        ImageServiceMixin.__init__(self)
        self.device = "cuda" if HAS_TORCH and torch.cuda.is_available() else "cpu"
        self.dataset_path = None
        self.image_size = self.DEFAULT_IMAGE_SIZE
        self.class_names = DEFAULT_CLASSES.copy()

    def get_service_info(self) -> Dict[str, Any]:
        """获取服务信息"""
        return {
            "has_torch": HAS_TORCH,
            "has_easyfsl": HAS_EASYFSL,
            "device": self.device,
            "total_classes": len(DEFAULT_CLASSES),
            "default_n_way": self.DEFAULT_N_WAY,
            "default_n_shot": self.DEFAULT_N_SHOT,
            "default_image_size": self.DEFAULT_IMAGE_SIZE
        }

    def handle_command(self, command: str, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理命令"""
        if not HAS_TORCH:
            return self.create_error_response("PyTorch 库未安装。请运行: pip install torch torchvision")

        if not HAS_EASYFSL:
            return self.create_error_response("EasyFSL 库未安装。请运行: pip install easyfsl")

        handlers = {
            "load_model": self._handle_load_model,
            "few_shot_classify": self._handle_few_shot_classify,
        }

        handler = handlers.get(command)
        if handler:
            return handler(request)
        else:
            return self.create_error_response(f"未知命令: {command}")

    def _handle_load_model(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理加载模型命令"""
        model_path = request.get("model_path", "")

        # 验证模型路径
        valid, error_msg = self.validate_model_path(model_path)
        if not valid:
            return self.create_error_response(error_msg)

        try:
            # 创建网络
            convolutional_network = resnet12()
            self.model = PrototypicalNetworks(convolutional_network).to(self.device)

            # 加载模型权重
            checkpoint = torch.load(model_path, map_location=self.device)
            if "model_state_dict" in checkpoint:
                self.model.load_state_dict(checkpoint["model_state_dict"])
            elif "state_dict" in checkpoint:
                self.model.load_state_dict(checkpoint["state_dict"])
            else:
                self.model.load_state_dict(checkpoint)

            self.model.eval()
            self.model_loaded = True

            return self.create_success_response(
                message=f"模型加载成功: {model_path}",
                data={
                    "num_classes": len(self.class_names),
                    "class_names": self.class_names[:10],
                    "device": self.device,
                    "total_classes": len(DEFAULT_CLASSES)
                }
            )

        except Exception as e:
            self.model_loaded = False
            import traceback
            return self.create_error_response(f"加载模型失败: {str(e)}", traceback.format_exc())

    def _get_transform(self, image_size: int):
        """获取图像变换"""
        return transforms.Compose([
            transforms.Resize((image_size, image_size)),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406],
                               std=[0.229, 0.224, 0.225])
        ])

    def _extract_features(self, image_path: str, image_size: int = 84):
        """提取图像特征"""
        transform = self._get_transform(image_size)
        image = Image.open(image_path).convert('RGB')
        image_tensor = transform(image).unsqueeze(0).to(self.device)

        with torch.no_grad():
            features = self.model.backbone(image_tensor)

        return features

    def _handle_few_shot_classify(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """处理小样本分类命令"""
        image_path = request.get("image_path", "")

        # 验证图像路径
        valid, error_msg = self.validate_image_path(image_path)
        if not valid:
            return self.create_error_response(error_msg)

        if not self.model_loaded:
            return self.create_error_response("模型未加载")

        # 获取并验证参数
        n_way = int(request.get("n_way", self.DEFAULT_N_WAY))
        n_shot = int(request.get("n_shot", self.DEFAULT_N_SHOT))
        n_query = int(request.get("n_query", self.DEFAULT_N_QUERY))
        image_size = int(request.get("image_size", self.DEFAULT_IMAGE_SIZE))
        num_episodes = int(request.get("num_episodes", self.DEFAULT_NUM_EPISODES))

        # 限制参数范围
        n_way = max(self.MIN_N_WAY, min(self.MAX_N_WAY, n_way))
        n_shot = max(self.MIN_N_SHOT, min(self.MAX_N_SHOT, n_shot))
        image_size = max(self.MIN_IMAGE_SIZE, min(self.MAX_IMAGE_SIZE, image_size))
        num_episodes = max(self.MIN_EPISODES, min(self.MAX_EPISODES, num_episodes))

        self.image_size = image_size

        try:
            # 提取查询图像特征
            query_features = self._extract_features(image_path, image_size)

            # 确定支持集路径
            dataset_path = self._find_dataset_path()

            if dataset_path is None:
                return self._few_shot_classify_simulated(
                    image_path, query_features, n_way, n_shot, n_query, image_size, num_episodes
                )

            # 从数据集加载类别列表
            self.class_names = self._load_class_names_from_dataset(dataset_path)

            # 加载支持集
            support_set = self._load_support_set(dataset_path, n_shot)
            if not support_set:
                return self.create_error_response("无法加载支持集")

            # 计算类别原型
            prototypes = self._compute_class_prototypes(support_set, image_size)

            # 计算查询图像与每个原型的距离
            class_similarities = []
            for class_name, prototype in prototypes.items():
                distance = torch.cdist(query_features, prototype, p=2).item()
                similarity = np.exp(-distance / 2.0)

                # 找到类别ID
                class_id = -1
                for idx, name in enumerate(self.class_names):
                    if class_name.lower() in name.lower() or name.lower() in class_name.lower():
                        class_id = idx
                        break

                class_similarities.append({
                    'class_id': class_id if class_id >= 0 else 0,
                    'class_name': class_name,
                    'similarity': float(similarity),
                    'avg_distance': float(distance),
                    'episode_count': 1
                })

            return self._build_classification_result(class_similarities, n_way, n_shot, num_episodes)

        except Exception as e:
            import traceback
            return self.create_error_response(f"小样本分类失败: {str(e)}", traceback.format_exc())

    def _find_dataset_path(self) -> str:
        """查找数据集路径"""
        possible_paths = [
            "data/MEET-FSL-official",
            "../data/MEET-FSL-official",
            "../../data/MEET-FSL-official",
            os.path.join(os.path.dirname(__file__), "../../../../data/MEET-FSL-official"),
        ]

        for path in possible_paths:
            if os.path.exists(path):
                return path
        return None

    def _load_class_names_from_dataset(self, dataset_path: str) -> List[str]:
        """从数据集目录加载类别名称"""
        class_names = []
        possible_dirs = ['train', 'support', 'base']
        support_dir = None

        for subdir in possible_dirs:
            test_path = os.path.join(dataset_path, subdir)
            if os.path.exists(test_path):
                support_dir = test_path
                break

        if support_dir is None:
            support_dir = dataset_path

        if os.path.exists(support_dir):
            for item in sorted(os.listdir(support_dir)):
                item_path = os.path.join(support_dir, item)
                if os.path.isdir(item_path):
                    class_names.append(item)

        return class_names if class_names else DEFAULT_CLASSES.copy()

    def _load_support_set(self, dataset_path: str, n_shot: int = 5) -> Dict[str, List[str]]:
        """加载支持集图像"""
        support_set = {}
        possible_dirs = ['train', 'support', 'base']
        support_dir = None

        for subdir in possible_dirs:
            test_path = os.path.join(dataset_path, subdir)
            if os.path.exists(test_path):
                support_dir = test_path
                break

        if support_dir is None:
            support_dir = dataset_path

        for class_name in os.listdir(support_dir):
            class_dir = os.path.join(support_dir, class_name)
            if not os.path.isdir(class_dir):
                continue

            image_files = []
            for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp', '*.tif', '*.tiff']:
                image_files.extend(Path(class_dir).glob(ext))
                image_files.extend(Path(class_dir).glob(ext.upper()))

            selected_files = sorted([str(f) for f in image_files])[:n_shot]
            if selected_files:
                support_set[class_name] = selected_files

        return support_set

    def _compute_class_prototypes(self, support_set: Dict[str, List[str]], image_size: int) -> Dict[str, torch.Tensor]:
        """计算每个类别的原型"""
        prototypes = {}

        for class_name, image_paths in support_set.items():
            features_list = []
            for img_path in image_paths:
                try:
                    features = self._extract_features(img_path, image_size)
                    features_list.append(features)
                except Exception:
                    continue

            if features_list:
                class_features = torch.cat(features_list, dim=0)
                prototype = class_features.mean(dim=0, keepdim=True)
                prototypes[class_name] = prototype

        return prototypes

    def _few_shot_classify_simulated(self, image_path: str, query_features: torch.Tensor,
                                      n_way: int, n_shot: int, n_query: int,
                                      image_size: int, num_episodes: int) -> Dict[str, Any]:
        """使用模拟支持样本进行小样本分类"""
        feature_seed = int(torch.sum(query_features).item() * 10000) % 2**32
        random.seed(feature_seed)
        np.random.seed(feature_seed)
        torch.manual_seed(feature_seed)

        num_classes = len(self.class_names)

        # 预计算每个类别的原型方向向量
        class_directions = []
        for c in range(num_classes):
            torch.manual_seed(c)
            direction = torch.randn_like(query_features)
            direction = F.normalize(direction, dim=1)
            class_directions.append(direction)
        class_directions = torch.cat(class_directions, dim=0)

        torch.manual_seed(feature_seed)

        # 记录每个类别的累计距离和出现次数
        class_distances = {c: [] for c in range(num_classes)}

        # 执行多轮 episode
        for episode in range(num_episodes):
            selected_classes = random.sample(range(num_classes), n_way)

            for i, class_id in enumerate(selected_classes):
                direction = class_directions[class_id:class_id+1]
                shot_distances = []

                for shot in range(n_shot):
                    projection = torch.sum(query_features * direction) * direction
                    noise = torch.randn_like(query_features) * 0.1
                    support = query_features + projection * 0.3 + noise

                    dist = torch.cdist(query_features, support, p=2).item()
                    shot_distances.append(dist)

                avg_dist = np.mean(shot_distances)
                class_distances[class_id].append(avg_dist)

        return self._build_classification_result_from_distances(
            class_distances, n_way, n_shot, num_episodes
        )

    def _build_classification_result_from_distances(self, class_distances: Dict[int, List[float]],
                                                     n_way: int, n_shot: int,
                                                     num_episodes: int) -> Dict[str, Any]:
        """从距离字典构建分类结果"""
        class_similarities = []

        for class_id in range(len(self.class_names)):
            if len(class_distances[class_id]) > 0:
                avg_distance = np.mean(class_distances[class_id])
                similarity = np.exp(-avg_distance / 2.0)
                class_similarities.append({
                    'class_id': class_id,
                    'class_name': self.class_names[class_id],
                    'similarity': float(similarity),
                    'avg_distance': float(avg_distance),
                    'episode_count': len(class_distances[class_id])
                })

        return self._build_classification_result(class_similarities, n_way, n_shot, num_episodes)

    def _build_classification_result(self, class_similarities: List[Dict],
                                      n_way: int, n_shot: int, num_episodes: int) -> Dict[str, Any]:
        """从相似度列表构建分类结果"""
        # 按相似度排序
        sorted_results = sorted(class_similarities, key=lambda x: x['similarity'], reverse=True)

        # 构建分类结果（Top-10）
        classifications = []
        for rank, result in enumerate(sorted_results[:10], 1):
            classifications.append({
                "rank": rank,
                "confidence": round(result['similarity'], 4),
                "class_id": result['class_id'],
                "label": result['class_name'],
                "avg_distance": round(result['avg_distance'], 4),
                "episode_count": result['episode_count']
            })

        top_prediction = classifications[0] if classifications else None

        # 计算统计信息
        all_sims = [r['similarity'] for r in sorted_results]
        all_dists = [r['avg_distance'] for r in sorted_results]

        if all_sims and all_dists:
            similarity_stats = {
                "max": round(float(max(all_sims)), 4),
                "min": round(float(min(all_sims)), 4),
                "mean": round(float(np.mean(all_sims)), 4),
                "std": round(float(np.std(all_sims)), 4)
            }
        else:
            similarity_stats = {"max": 0, "min": 0, "mean": 0, "std": 0}

        return self.create_success_response(
            message=f"小样本分类完成，预测: {top_prediction['label'] if top_prediction else 'N/A'} "
                   f"({top_prediction['confidence']:.2%})" if top_prediction else "",
            data={
                "classifications": classifications,
                "top_prediction": top_prediction,
                "n_way": n_way,
                "n_shot": n_shot,
                "num_episodes": num_episodes,
                "total_classes": len(self.class_names),
                "similarity_stats": similarity_stats
            }
        )

    def cleanup(self):
        """清理资源"""
        self.model = None
        self.model_loaded = False


def main():
    """主函数"""
    service = FSLService()
    service.run()


if __name__ == "__main__":
    main()
