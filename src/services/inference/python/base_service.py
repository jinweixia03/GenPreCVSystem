#!/usr/bin/env python3
"""
深度学习服务基类

提供通用的服务功能，包括：
- JSON IPC 通信协议
- 请求/响应处理
- 错误处理和日志
- 服务生命周期管理

子类需要实现:
- handle_command(command, request) -> dict
"""

import sys
import json
import os
from abc import ABC, abstractmethod
from typing import Dict, Any

# 修复 OpenMP 库冲突问题（必须在导入其他库之前设置）
os.environ['KMP_DUPLICATE_LIB_OK'] = 'TRUE'


class BaseService(ABC):
    """深度学习服务基类"""

    # 服务配置常量
    VERSION = "1.0.0"
    DEFAULT_ENCODING = 'utf-8'
    MAX_REQUEST_SIZE = 10 * 1024 * 1024  # 10MB 最大请求大小

    def __init__(self, service_name: str):
        self.service_name = service_name
        self.running = False
        self.request_count = 0
        self.error_count = 0

    @abstractmethod
    def handle_command(self, command: str, request: Dict[str, Any]) -> Dict[str, Any]:
        """
        处理具体命令

        Args:
            command: 命令名称
            request: 请求参数字典

        Returns:
            响应字典
        """
        pass

    @abstractmethod
    def get_service_info(self) -> Dict[str, Any]:
        """
        获取服务信息

        Returns:
            服务信息字典
        """
        pass

    def validate_request(self, request: Dict[str, Any]) -> tuple[bool, str]:
        """
        验证请求格式

        Args:
            request: 请求字典

        Returns:
            (是否有效, 错误信息)
        """
        if not isinstance(request, dict):
            return False, "请求必须是 JSON 对象"

        if "command" not in request:
            return False, "缺少必需的 'command' 字段"

        command = request.get("command")
        if not isinstance(command, str):
            return False, "'command' 必须是字符串"

        if len(command) > 100:
            return False, "命令名称过长"

        return True, ""

    def create_success_response(self, message: str = "", data: Dict[str, Any] = None) -> Dict[str, Any]:
        """创建成功响应"""
        response = {
            "success": True,
            "message": message
        }
        if data is not None:
            response["data"] = data
        return response

    def create_error_response(self, message: str, error_detail: str = None) -> Dict[str, Any]:
        """创建错误响应"""
        response = {
            "success": False,
            "message": message
        }
        if error_detail:
            response["error_detail"] = error_detail
        return response

    def send_response(self, response: Dict[str, Any]):
        """发送响应到 stdout"""
        print(json.dumps(response, ensure_ascii=False), flush=True)

    def run(self):
        """运行服务主循环"""
        self.running = True

        # 发送就绪信号
        ready_response = self.create_success_response(
            message=f"{self.service_name} 服务已启动",
            data={
                "version": self.VERSION,
                **self.get_service_info()
            }
        )
        self.send_response(ready_response)

        # 主循环
        try:
            for line in sys.stdin:
                line = line.strip()
                if not line:
                    continue

                # 检查请求大小
                if len(line) > self.MAX_REQUEST_SIZE:
                    self.send_response(self.create_error_response("请求数据过大"))
                    continue

                self.request_count += 1

                try:
                    request = json.loads(line)
                except json.JSONDecodeError as e:
                    self.error_count += 1
                    self.send_response(self.create_error_response(f"JSON 解析错误: {str(e)}"))
                    continue

                # 验证请求
                valid, error_msg = self.validate_request(request)
                if not valid:
                    self.error_count += 1
                    self.send_response(self.create_error_response(error_msg))
                    continue

                command = request.get("command", "")

                # 处理退出命令
                if command == "exit":
                    self.send_response(self.create_success_response("服务已停止"))
                    self.running = False
                    break

                # 处理具体命令
                try:
                    response = self.handle_command(command, request)
                    self.send_response(response)
                except Exception as e:
                    self.error_count += 1
                    import traceback
                    self.send_response(self.create_error_response(
                        f"处理错误: {str(e)}",
                        traceback.format_exc()
                    ))

        except KeyboardInterrupt:
            self.send_response(self.create_success_response("服务被中断"))
        except Exception as e:
            import traceback
            self.send_response(self.create_error_response(
                f"服务错误: {str(e)}",
                traceback.format_exc()
            ))
        finally:
            self.cleanup()

    def cleanup(self):
        """清理资源，子类可重写"""
        pass


class ModelServiceMixin:
    """模型服务混入类，提供通用的模型加载功能"""

    def __init__(self):
        self.model = None
        self.model_loaded = False
        self.model_path = None
        self.class_names = []

    def validate_model_path(self, model_path: str) -> tuple[bool, str]:
        """验证模型路径"""
        if not model_path:
            return False, "模型路径为空"

        if len(model_path) > 4096:
            return False, "路径过长"

        # 检查路径遍历攻击
        normalized_path = os.path.normpath(model_path)
        if normalized_path.startswith('..') or normalized_path.startswith('/..'):
            return False, "非法路径"

        if not os.path.exists(model_path):
            return False, f"模型文件不存在: {model_path}"

        if not os.path.isfile(model_path):
            return False, "路径不是文件"

        # 检查文件扩展名
        valid_extensions = ('.pt', '.pth', '.onnx', '.engine', '.mlmodel')
        if not any(model_path.lower().endswith(ext) for ext in valid_extensions):
            return False, f"不支持的模型格式，必须是以下之一: {valid_extensions}"

        # 检查文件大小（最大 5GB）
        max_size = 5 * 1024 * 1024 * 1024
        try:
            file_size = os.path.getsize(model_path)
            if file_size > max_size:
                return False, f"模型文件过大 ({file_size / (1024**3):.2f} GB > 5 GB)"
            if file_size == 0:
                return False, "模型文件为空"
        except OSError as e:
            return False, f"无法访问模型文件: {e}"

        return True, ""

    def load_class_names(self, labels_path: str = None) -> list:
        """加载类别名称"""
        if labels_path and os.path.exists(labels_path):
            try:
                with open(labels_path, 'r', encoding='utf-8') as f:
                    return [line.strip() for line in f if line.strip()]
            except Exception as e:
                print(f"警告: 无法加载标签文件: {e}", file=sys.stderr)
        return []


class ImageServiceMixin:
    """图像服务混入类，提供通用的图像处理功能"""

    # 支持的图像格式
    SUPPORTED_IMAGE_EXTENSIONS = ('.jpg', '.jpeg', '.png', '.bmp', '.gif', '.tiff', '.tif', '.webp')

    # 最大图像尺寸 (100MP)
    MAX_IMAGE_PIXELS = 100 * 1024 * 1024

    # 最大文件大小 (1GB)
    MAX_IMAGE_SIZE = 1024 * 1024 * 1024

    def validate_image_path(self, image_path: str) -> tuple[bool, str]:
        """验证图像路径"""
        if not image_path:
            return False, "图像路径为空"

        if len(image_path) > 4096:
            return False, "路径过长"

        # 检查路径遍历攻击
        normalized_path = os.path.normpath(image_path)
        if normalized_path.startswith('..') or normalized_path.startswith('/..'):
            return False, "非法路径"

        if not os.path.exists(image_path):
            return False, f"图像文件不存在: {image_path}"

        if not os.path.isfile(image_path):
            return False, "路径不是文件"

        # 检查文件扩展名
        ext = os.path.splitext(image_path.lower())[1]
        if ext not in self.SUPPORTED_IMAGE_EXTENSIONS:
            return False, f"不支持的图像格式: {ext}"

        # 检查文件大小
        try:
            file_size = os.path.getsize(image_path)
            if file_size == 0:
                # 文件可能正在写入，等待重试
                import time
                time.sleep(0.1)
                file_size = os.path.getsize(image_path)

            if file_size == 0:
                return False, "图像文件为空或正在写入"

            if file_size > self.MAX_IMAGE_SIZE:
                size_gb = file_size / (1024**3)
                return False, f"图像文件过大 ({size_gb:.2f} GB > 1 GB)"

        except OSError as e:
            return False, f"无法访问图像文件: {e}"

        return True, ""

    def get_image_info(self, image_path: str) -> Dict[str, Any]:
        """获取图像信息"""
        try:
            from PIL import Image
            with Image.open(image_path) as img:
                return {
                    "width": img.width,
                    "height": img.height,
                    "format": img.format,
                    "mode": img.mode,
                    "size_bytes": os.path.getsize(image_path)
                }
        except Exception as e:
            return {"error": str(e)}
