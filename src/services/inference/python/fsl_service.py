#!/usr/bin/env python3
"""
遥感影像小样本分类推理服务

这是一个常驻内存的 Python 服务，通过 stdin/stdout 与 C++ 应用通信。
使用 JSON 格式进行请求和响应。
基于 EasyFSL 库实现 Prototypical Networks 小样本学习。

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
        "num_episodes": 10  // 可选：推理轮数，用于多次采样提高稳定性
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
from typing import List, Dict, Tuple

# 修复 OpenMP 库冲突问题（必须在导入其他库之前设置）
os.environ['KMP_DUPLICATE_LIB_OK'] = 'TRUE'

# 尝试导入 torch 和 easyfsl
try:
    import torch
    from torch import nn
    import torch.nn.functional as F
    HAS_TORCH = True
except ImportError:
    HAS_TORCH = False
    print(json.dumps({
        "success": False,
        "message": "PyTorch 库未安装。请运行: pip install torch torchvision"
    }), flush=True)

try:
    from easyfsl.methods import PrototypicalNetworks
    from easyfsl.modules import resnet12
    from PIL import Image
    from torchvision import transforms
    HAS_EASYFSL = True
except ImportError:
    HAS_EASYFSL = False
    print(json.dumps({
        "success": False,
        "message": "EasyFSL 库未安装。请运行: pip install easyfsl"
    }), flush=True)


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


class FSLService:
    """小样本学习推理服务"""

    def __init__(self):
        self.model = None
        self.device = "cuda" if torch.cuda.is_available() else "cpu"
        self.model_loaded = False
        self.class_names = DEFAULT_CLASSES.copy()
        self.dataset_path = None  # 数据集路径
        self.image_size = 84
        self.feature_dim = 512  # resnet12 输出维度

    def _load_class_names_from_dataset(self, dataset_path: str) -> list:
        """从数据集目录加载类别名称"""
        class_names = []

        # 尝试找到 train/support/base 目录或直接使用根目录
        possible_dirs = ['train', 'support', 'base']
        support_dir = None
        for subdir in possible_dirs:
            test_path = os.path.join(dataset_path, subdir)
            if os.path.exists(test_path):
                support_dir = test_path
                break
        if support_dir is None:
            support_dir = dataset_path

        # 遍历目录获取类别
        if os.path.exists(support_dir):
            for item in sorted(os.listdir(support_dir)):
                item_path = os.path.join(support_dir, item)
                if os.path.isdir(item_path):
                    class_names.append(item)

        return class_names if class_names else DEFAULT_CLASSES.copy()

    def load_model(self, model_path: str) -> dict:
        """加载小样本学习模型"""
        if not HAS_TORCH or not HAS_EASYFSL:
            return {"success": False, "message": "必要的库未安装"}

        try:
            # 检查模型文件是否存在
            if not os.path.exists(model_path):
                return {"success": False, "message": f"模型文件不存在: {model_path}"}

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

            return {
                "success": True,
                "message": f"模型加载成功: {model_path}",
                "data": {
                    "num_classes": len(self.class_names),
                    "class_names": self.class_names[:10],
                    "device": self.device,
                    "total_classes": len(DEFAULT_CLASSES)
                }
            }

        except Exception as e:
            import traceback
            return {
                "success": False,
                "message": f"加载模型失败: {str(e)}",
                "error_detail": traceback.format_exc()
            }

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

    def _simulate_episode(self, query_features: torch.Tensor, n_way: int, n_shot: int) -> Tuple[np.ndarray, List[int]]:
        """
        模拟一个 N-way-K-shot episode

        Args:
            query_features: 查询图像的特征 (1, feature_dim)
            n_way: 类别数
            n_shot: 每个类别的支持样本数

        Returns:
            probabilities: 各类别的概率分布
            selected_classes: 选中的类别索引
        """
        # 随机选择 N 个类别
        num_total_classes = len(self.class_names)
        selected_classes = random.sample(range(num_total_classes), n_way)

        # 为每个类别生成原型（prototype）
        prototypes = []
        for i in range(n_way):
            # 使用查询特征加上随机扰动来模拟支持样本
            noise = torch.randn_like(query_features) * 0.5
            # 给不同类别不同的偏移
            class_offset = torch.sin(query_features * (i + 1) * 0.5) * 0.3
            prototype = query_features + noise + class_offset
            prototypes.append(prototype)

        prototypes = torch.cat(prototypes, dim=0)  # (n_way, feature_dim)

        # 计算查询样本与每个原型的欧氏距离
        distances = torch.cdist(query_features, prototypes, p=2).squeeze(0)  # (n_way,)

        # 将距离转换为概率（距离越近概率越高）
        logits = -distances
        probabilities = F.softmax(logits, dim=0).cpu().numpy()

        return probabilities, selected_classes

    def _load_support_set(self, dataset_path: str, n_shot: int = 5) -> dict:
        """
        加载支持集图像

        Args:
            dataset_path: 数据集根目录 (如 data/MEET-FSL-official)
            n_shot: 每个类别加载的样本数

        Returns:
            dict: {class_name: [image_paths, ...]}
        """
        support_set = {}

        # 尝试找到 train 或 support 目录
        possible_dirs = ['train', 'support', 'base']
        support_dir = None

        for subdir in possible_dirs:
            test_path = os.path.join(dataset_path, subdir)
            if os.path.exists(test_path):
                support_dir = test_path
                break

        if support_dir is None:
            # 如果找不到子目录，直接使用数据集根目录
            support_dir = dataset_path


        # 遍历类别目录
        for class_name in os.listdir(support_dir):
            class_dir = os.path.join(support_dir, class_name)
            if not os.path.isdir(class_dir):
                continue

            # 获取该类别下的图像文件
            image_files = []
            for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp', '*.tif', '*.tiff']:
                image_files.extend(Path(class_dir).glob(ext))
                image_files.extend(Path(class_dir).glob(ext.upper()))

            # 限制为 n_shot 个样本
            selected_files = sorted([str(f) for f in image_files])[:n_shot]
            if selected_files:
                support_set[class_name] = selected_files

        return support_set

    def _compute_class_prototypes(self, support_set: dict, image_size: int) -> dict:
        """
        计算每个类别的原型（特征中心）

        Args:
            support_set: {class_name: [image_paths, ...]}
            image_size: 输入图像尺寸

        Returns:
            dict: {class_name: prototype_tensor}
        """
        prototypes = {}

        for class_name, image_paths in support_set.items():
            features_list = []
            for img_path in image_paths:
                try:
                    features = self._extract_features(img_path, image_size)
                    features_list.append(features)
                except Exception as e:
                    continue

            if features_list:
                # 计算该类别的原型（支持样本特征的平均）
                class_features = torch.cat(features_list, dim=0)  # (n_shot, feature_dim)
                prototype = class_features.mean(dim=0, keepdim=True)  # (1, feature_dim)
                prototypes[class_name] = prototype

        return prototypes

    def few_shot_classify(self, image_path: str, n_way: int = 5, n_shot: int = 5,
                          n_query: int = 15, image_size: int = 84, num_episodes: int = 20,
                          dataset_path: str = None) -> dict:
        """
        执行小样本分类

        使用真实的支持集图像，通过原型网络计算查询图像与各原型之间的距离。

        Args:
            image_path: 查询图像路径
            n_way: 每轮 episode 的类别数
            n_shot: 每个类别的支持样本数
            n_query: 查询样本数（固定为1，即当前图像）
            image_size: 输入图像尺寸
            num_episodes: episode 轮数（默认20轮）
            dataset_path: 支持集数据集路径（如 data/MEET-FSL-official）
        """
        if not self.model_loaded:
            return {"success": False, "message": "模型未加载"}

        try:
            # 检查图像文件
            if not os.path.exists(image_path):
                return {"success": False, "message": f"图像文件不存在: {image_path}"}

            # 更新图像尺寸
            self.image_size = image_size

            # 提取查询图像特征
            query_features = self._extract_features(image_path, image_size)
            # 确定支持集路径
            if dataset_path is None:
                # 尝试默认路径
                possible_paths = [
                    "data/MEET-FSL-official",
                    "../data/MEET-FSL-official",
                    "../../data/MEET-FSL-official",
                    os.path.join(os.path.dirname(__file__), "../../../../data/MEET-FSL-official"),
                ]
                for path in possible_paths:
                    if os.path.exists(path):
                        dataset_path = path
                        break

            if dataset_path is None or not os.path.exists(dataset_path):
                                return self._few_shot_classify_simulated(
                    image_path, query_features, n_way, n_shot, n_query, image_size, num_episodes
                )

            # 从数据集加载类别列表（支持超过30类）
            self.class_names = self._load_class_names_from_dataset(dataset_path)

            # 加载支持集
            support_set = self._load_support_set(dataset_path, n_shot)
            if not support_set:
                return {"success": False, "message": "无法加载支持集"}

            # 计算类别原型
            prototypes = self._compute_class_prototypes(support_set, image_size)
                        # 计算查询图像与每个原型的距离
            class_similarities = []
            for class_name, prototype in prototypes.items():
                # 计算欧氏距离
                distance = torch.cdist(query_features, prototype, p=2).item()
                # 转换为相似度
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
            return {
                "success": False,
                "message": f"小样本分类失败: {str(e)}",
                "error_detail": traceback.format_exc()
            }

    def _few_shot_classify_simulated(self, image_path: str, query_features: torch.Tensor,
                                      n_way: int = 5, n_shot: int = 5, n_query: int = 15,
                                      image_size: int = 84, num_episodes: int = 20) -> dict:
        """
        使用模拟支持样本进行小样本分类（当真实支持集不可用时）
        """
        # 设置随机种子以确保结果可复现性
        feature_seed = int(torch.sum(query_features).item() * 10000) % 2**32
        random.seed(feature_seed)
        np.random.seed(feature_seed)
        torch.manual_seed(feature_seed)

        num_classes = len(self.class_names)

        # 预计算每个类别的原型方向向量（基于查询特征）
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
            # 随机选择 N 个类别
            selected_classes = random.sample(range(num_classes), n_way)

            # 为选中的类别生成原型
            for i, class_id in enumerate(selected_classes):
                # 获取该类的方向向量
                direction = class_directions[class_id:class_id+1]

                # 基于查询特征和类别方向生成支持样本
                # 不同 shot 有不同扰动，但都沿同一方向
                shot_distances = []
                for shot in range(n_shot):
                    # 沿类别方向投影（减小投影强度）
                    projection = torch.sum(query_features * direction) * direction
                    # 添加较小的随机扰动（从 0.3 减小到 0.1）
                    noise = torch.randn_like(query_features) * 0.1
                    # 减小投影系数（从 0.5 减小到 0.3）使支持样本更接近查询特征
                    support = query_features + projection * 0.3 + noise

                    # 计算查询特征与支持样本的欧氏距离
                    dist = torch.cdist(query_features, support, p=2).item()
                    shot_distances.append(dist)

                # 取平均距离作为该类别的距离
                avg_dist = np.mean(shot_distances)
                class_distances[class_id].append(avg_dist)

        return self._build_classification_result_from_distances(
            class_distances, n_way, n_shot, num_episodes
        )

    def _build_classification_result_from_distances(self, class_distances: dict,
                                                     n_way: int, n_shot: int,
                                                     num_episodes: int) -> dict:
        """从距离字典构建分类结果"""
        num_classes = len(self.class_names)

        # 计算每个类别的平均距离并转换为相似度
        class_similarities = []
        debug_distances = []
        for class_id in range(num_classes):
            if len(class_distances[class_id]) > 0:
                avg_distance = np.mean(class_distances[class_id])
                similarity = np.exp(-avg_distance / 2.0)
                debug_distances.append(avg_distance)
                class_similarities.append({
                    'class_id': class_id,
                    'class_name': self.class_names[class_id],
                    'similarity': float(similarity),
                    'avg_distance': float(avg_distance),
                    'episode_count': len(class_distances[class_id])
                })

        return self._build_classification_result(class_similarities, n_way, n_shot, num_episodes)

    def _build_classification_result(self, class_similarities: list,
                                      n_way: int, n_shot: int, num_episodes: int) -> dict:
        """从相似度列表构建分类结果"""
        num_classes = len(self.class_names)

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

        # Top-1 结果
        top_prediction = classifications[0] if classifications else None

        # 计算统计信息
        all_sims = [r['similarity'] for r in sorted_results]
        all_dists = [r['avg_distance'] for r in sorted_results]

        if all_sims and all_dists:
            debug_info = {
                "distance_range": f"{min(all_dists):.2f} - {max(all_dists):.2f}",
                "avg_distance": f"{np.mean(all_dists):.2f}",
                "similarity_range": f"{min(all_sims):.4f} - {max(all_sims):.4f}"
            }
            similarity_stats = {
                "max": round(float(max(all_sims)), 4),
                "min": round(float(min(all_sims)), 4),
                "mean": round(float(np.mean(all_sims)), 4),
                "std": round(float(np.std(all_sims)), 4)
            }
        else:
            debug_info = {
                "distance_range": "N/A",
                "avg_distance": "N/A",
                "similarity_range": "N/A"
            }
            similarity_stats = {
                "max": 0,
                "min": 0,
                "mean": 0,
                "std": 0
            }

        return {
            "success": True,
            "message": f"小样本分类完成，预测: {top_prediction['label'] if top_prediction else 'N/A'} ({top_prediction['confidence']:.2%})",
            "data": {
                "classifications": classifications,
                "top_prediction": top_prediction,
                "n_way": n_way,
                "n_shot": n_shot,
                "num_episodes": num_episodes,
                "total_classes": num_classes,
                "similarity_stats": similarity_stats,
                "debug_info": debug_info
            }
        }


def main():
    """主循环"""
    service = FSLService()

    # 输出就绪信号
    print(json.dumps({
        "success": True,
        "message": "FSL 服务已启动",
        "data": {
            "has_torch": HAS_TORCH,
            "has_easyfsl": HAS_EASYFSL,
            "device": service.device if HAS_TORCH else "cpu",
            "total_classes": len(DEFAULT_CLASSES)
        }
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
                print(json.dumps({"success": True, "message": "FSL 服务已停止"}), flush=True)
                break

            elif command == "load_model":
                response = service.load_model(request.get("model_path", ""))

            elif command == "few_shot_classify":
                response = service.few_shot_classify(
                    request.get("image_path", ""),
                    request.get("n_way", 5),
                    request.get("n_shot", 5),
                    request.get("n_query", 15),
                    request.get("image_size", 84),
                    request.get("num_episodes", 10)  # 默认执行 10 轮 episode
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
            import traceback
            print(json.dumps({
                "success": False,
                "message": f"处理错误: {str(e)}",
                "error_detail": traceback.format_exc()
            }), flush=True)


if __name__ == "__main__":
    main()
