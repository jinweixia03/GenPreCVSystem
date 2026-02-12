# 模型文件目录

此目录用于存放各任务类型的模型文件。

## 目录结构

```
models/
├── detection/         # 目标检测模型
│   ├── yolov8n.pt
│   ├── yolov8s.pt
│   └── yolov11n.pt
│
├── segmentation/      # 实例分割模型
│   ├── yolov8n-seg.pt
│   └── yolov11n-seg.pt
│
├── classification/    # 图像分类模型
│   ├── yolov8n-cls.pt
│   └── resnet50.pt
│
└── keypoint/          # 关键点检测模型
    ├── yolov8n-pose.pt
    └── yolov11n-pose.pt
```

## 模型下载

### YOLOv8 模型
- 检测: https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8n.pt
- 分割: https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8n-seg.pt
- 分类: https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8n-cls.pt
- 姿态: https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8n-pose.pt

### YOLOv11 模型
- 检测: https://github.com/ultralytics/assets/releases/download/v11.0/yolo11n.pt
- 分割: https://github.com/ultralytics/assets/releases/download/v11.0/yolo11n-seg.pt
- 分类: https://github.com/ultralytics/assets/releases/download/v11.0/yolo11n-cls.pt
- 姿态: https://github.com/ultralytics/assets/releases/download/v11.0/yolo11n-pose.pt

## 支持的模型格式

- `.pt` - PyTorch 模型
- `.pth` - PyTorch 模型
- `.onnx` - ONNX 模型
- `.torchscript` - TorchScript 模型
