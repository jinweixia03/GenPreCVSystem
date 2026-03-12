<div align="center">

# GenPreCVSystem

**Industrial Computer Vision Preprocessing System**

**工业级计算机视觉预处理系统**

[![Qt](https://img.shields.io/badge/Qt-6.9.3-41CD52?style=flat-square&logo=qt)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-00599C?style=flat-square&logo=c%2B%2B)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.16+-064F8C?style=flat-square&logo=cmake)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?style=flat-square&logo=windows)](https://www.microsoft.com/windows)
[![License](https://img.shields.io/badge/License-Copyright-red?style=flat-square)](LICENSE)

<img src="docs/images/screenshot.png" alt="Screenshot" width="80%" />

</div>

---

## Language / 语言

<details>
<summary><b>English</b></summary>

## Overview

GenPreCVSystem is an industrial-grade computer vision preprocessing application built with Qt 6.9 and C++17. It provides a comprehensive solution for image preprocessing, CV task configuration, and deep learning model inference through Python backend integration.

## Features

### Deep Learning Model Inference
- **Object Detection** - Detect objects with bounding boxes using YOLOv8/v9/v10/v11
- **Instance Segmentation** - Pixel-level instance segmentation with mask polygons
- **Image Classification** - Top-K classification results with confidence scores
- **Keypoint Detection** - Pose estimation and keypoint detection
- **Multi-environment Support** - Auto-detect and switch between Conda/virtualenv environments
- **Model Management** - Organized model storage by task type

### Image Processing
- **Basic Operations**: Grayscale, Invert, Binarize
- **Transformations**: Rotate (90° L/R), Flip (H/V)
- **Filters**: Blur, Sharpen, Edge Detection (Sobel, Canny, Laplacian)
- **Adjustments**: Brightness, Contrast, Saturation

### File Management
- **Multi-tab Support** - Edit multiple images simultaneously
- **File Tree Browser** - Left panel with directory navigation
- **Recent Files** - Quick access to recently opened files
- **Drag & Drop** - Support for files and folders
- **Export** - Save results in multiple formats (JSON, XML, CSV, Images)

### Batch Processing
- **Folder Batch Processing** - Process entire directories
- **Progress Tracking** - Real-time progress with cancel support
- **Result Summary** - Statistics and detailed logs

### User Interface
- **Splash Screen** - Loading screen with initialization progress
- **Undo/Redo** - 50-step history per tab
- **Clipboard Operations** - Copy/paste images
- **Keyboard Shortcuts** - Efficient workflow shortcuts
- **Real-time Log** - Output panel for operation logs

## Quick Start

### Prerequisites
- Windows 10/11
- Visual Studio 2022
- Qt 6.9.3 (msvc2022_64)
- CMake 3.16+
- Python 3.8+ with Conda (for YOLO inference)

### Python Environment Setup

Create and configure the Conda environment for YOLO inference:

```bash
# Create environment
conda create -n GenPreCVSystem python=3.10
conda activate GenPreCVSystem

# Install ultralytics
pip install ultralytics

# Or use the environment scanner to auto-detect existing environments
```

### Build

```bash
# Configure with CMake preset
cmake --preset msvc-release

# Build
cmake --build build --config Release
```

### Run

```bash
# Run the application
./build/Release/GenPreCVSystem.exe
```

### Deploy

```batch
deploy.bat
```

The executable will be at: `dist/GenPreCVSystem/GenPreCVSystem.exe`

## Project Structure

```
GenPreCVSystem/
├── src/                           # Source code
│   ├── core/                      # Application core
│   │   ├── main.cpp               # Entry point
│   │   ├── splashscreen.*         # Splash screen
│   │   └── application.h          # Application setup
│   ├── config/                    # Configuration management
│   │   ├── appsettings.*          # Application settings
│   │   ├── recentfilesmanager.*   # Recent files tracking
│   │   └── tasktypes.h            # Task type definitions
│   ├── models/                    # Data models
│   │   ├── tabdata.h              # Tab data model
│   │   ├── undostack.h            # Undo/redo stack
│   │   └── pythonenvironment.h    # Python environment model
│   ├── services/                  # Business logic services
│   │   ├── environment/           # Python environment management
│   │   │   ├── environmentcachemanager.*  # Environment caching
│   │   │   └── environmentscanner.*       # Environment detection
│   │   ├── image/                 # Image processing services
│   │   │   ├── imageprocessor.*   # Basic image operations
│   │   │   └── imageprocessservice.*  # Image service facade
│   │   ├── inference/             # Model inference services
│   │   │   ├── dlservice.*        # DL Python backend service
│   │   │   └── python/            # Python scripts
│   │   │       └── dl_service.py      # DL backend script
│   │   ├── io/                    # File I/O services
│   │   │   ├── fileutils.*        # File utilities
│   │   │   └── exportservice.*    # Export functionality
│   │   └── system/                # System services
│   │       ├── clipboardhelper.*  # Clipboard operations
│   │       ├── exceptions.*       # Exception handling
│   │       └── errordialog.*      # Error dialogs
│   ├── views/                     # UI views
│   │   ├── mainwindow.*           # Main window
│   │   ├── imageview.*            # Image viewer component
│   │   ├── imagefilefilterproxy.* # File filtering
│   │   ├── dialogs/               # Dialog windows
│   │   │   ├── batchprocessdialog.*   # Batch processing
│   │   │   ├── detectionresultdialog.* # Detection results
│   │   │   └── settingsdialog.*   # Application settings
│   │   └── components/            # Reusable UI components
│   │       ├── filetreeview.*     # File tree widget
│   │       └── environmentservicewidget.* # Environment selector
│   ├── controllers/               # MVC controllers
│   │   ├── tabcontroller.*        # Tab management
│   │   ├── imagecontroller.*      # Image operations
│   │   ├── filecontroller.*       # File operations
│   │   ├── taskcontroller.*       # Task execution
│   │   └── parameterpanelfactory.* # Parameter UI factory
│   └── resources/                 # Resources
│       ├── models/                # Model files (git-ignored)
│       │   ├── classification/    # Classification models
│       │   ├── detection/         # Detection models
│       │   ├── segmentation/      # Segmentation models
│       │   └── keypoint/          # Keypoint models
│       └── *.ts                   # Translations
├── tests/                         # Unit and integration tests
├── docs/                          # Documentation
├── third_party/                   # Third-party libraries
├── build/                         # Build directory
├── dist/                          # Distribution output
├── CMakeLists.txt                 # CMake configuration
└── deploy.bat                     # Deployment script
```

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Open Image | `Ctrl+O` |
| Open Folder | `Ctrl+D` |
| Close Tab | `Ctrl+W` |
| Save | `Ctrl+S` |
| Save As | `Ctrl+Shift+S` |
| Copy | `Ctrl+C` |
| Paste | `Ctrl+V` |
| Rotate Left | `Ctrl+L` |
| Rotate Right | `Ctrl+R` |
| Flip Horizontal | `Ctrl+H` |
| Flip Vertical | `Ctrl+Shift+H` |
| Zoom In | `Ctrl++` |
| Zoom Out | `Ctrl+-` |
| Fit Window | `Ctrl+F` |
| Actual Size | `Ctrl+1` |
| Run Task | `F5` |
| Stop Task | `Shift+F5` |
| Settings | `Ctrl+,` |
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` |

## Usage Guide

### Opening Images
1. **Menu**: File → Open Image (`Ctrl+O`)
2. **Folder**: File → Open Folder (`Ctrl+D`), then double-click image
3. **Drag & Drop**: Drag files/folders to file browser
4. **Recent Files**: Access from File → Recent Files menu

### YOLO Inference
1. **Select Python Environment**: Use the environment selector in the right panel
2. **Choose Task Type**: Select Detection/Segmentation/Classification/Keypoint
3. **Load Model**: Select a model file (.pt or .onnx)
4. **Adjust Parameters**: Set confidence threshold, IoU threshold, image size
5. **Run**: Click Run (`F5`) or press the run button
6. **View Results**: Detection results are displayed with bounding boxes/masks
7. **Export**: Save results as JSON, XML, CSV, or annotated images

### Batch Processing
1. **Open Batch Dialog**: Tools → Batch Processing
2. **Select Input Folder**: Choose directory with images
3. **Configure Settings**: Select model and parameters
4. **Start Processing**: Click Start and monitor progress
5. **Review Results**: View summary and export as needed

### Managing Settings
1. **Open Settings**: File → Settings (`Ctrl+,`)
2. **Configure Options**:
   - General: Language, theme, auto-save
   - Processing: Default thresholds, output formats
   - Python: Default environment paths
   - Shortcuts: Custom keyboard shortcuts
3. **Save**: Settings are persisted automatically

## YOLO Service Architecture

The YOLO inference service uses a Qt C++ frontend with Python backend architecture:

```
┌─────────────────┐     QProcess      ┌──────────────────┐
│   DLService     │ ◄─── (JSON IPC) ─►│   dl_service.py  │
│   (C++/Qt)      │                   │  (Python/Ultralytics)
└─────────────────┘                   └──────────────────┘
         │                                       │
         ▼                                       ▼
┌─────────────────┐                      ┌──────────────────┐
│   Qt Signals    │                      │   DL Models      │
│  (UI Updates)   │                      │  (PyTorch/onnx)  │
└─────────────────┘                      └──────────────────┘
```

### Features:
- **Environment Auto-Discovery**: Automatically scans for Conda/virtualenv environments
- **Caching**: Environment validation results are cached for fast startup
- **Async Operations**: Non-blocking environment scanning
- **Error Handling**: Graceful fallback and detailed error messages

### Example: Using DLService

```cpp
#include "services/inference/dlservice.h"

using namespace GenPreCVSystem::Utils;

DLService* service = new DLService(this);

// Scan available Python environments
QVector<PythonEnvironment> envs = service->scanEnvironments();

// Set environment and start service
service->setEnvironmentPath("/path/to/python.exe");
if (service->start()) {
    // Load model
    if (service->loadModel("yolov8n.pt")) {
        // Run detection
        YOLODetectionResult result = service->detect(
            "image.jpg",
            0.25f,  // confidence threshold
            0.45f,  // IoU threshold
            640     // image size
        );

        if (result.success) {
            for (const auto& det : result.detections) {
                qDebug() << "Detected:" << det.label
                         << "Confidence:" << det.confidence;
            }
        }
    }
}
```

## Supported YOLO Models

- **Detection**: YOLOv8n/s/m/l/x, YOLOv9, YOLOv10, YOLOv11, YOLOv26
- **Segmentation**: YOLOv8n-seg, YOLOv8s-seg, etc.
- **Classification**: YOLOv8n-cls, YOLOv8s-cls, etc.
- **Pose/Keypoint**: YOLOv8n-pose, YOLOv8s-pose, etc.

Models are organized in `src/resources/models/` by task type.

## Tech Stack

| Component | Technology |
|-----------|------------|
| Framework | Qt 6.9.3 |
| Language | C++17 |
| Build System | CMake 3.16+ |
| Compiler | MSVC 2022 |
| Python Backend | Ultralytics YOLO |
| Architecture | MVC Pattern |

## Testing

Run unit and integration tests:

```bash
# Build with tests enabled
cmake --preset msvc-release -DBUILD_TESTS=ON
cmake --build build --config Release

# Run tests
ctest --test-dir build -C Release
```

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

Copyright © 2024-2025 GenPreCVSystem. All rights reserved.

</details>

<details>
<summary><b>中文</b></summary>

## 项目概述

GenPreCVSystem 是一款基于 Qt 6.9 和 C++17 开发的工业级计算机视觉预处理应用程序，通过 Python 后端集成提供图像预处理、CV 任务配置和 YOLO 模型推理的综合解决方案。

## 功能特性

### YOLO 模型推理
- **目标检测** - 使用 YOLOv8/v9/v10/v11 进行边界框目标检测
- **实例分割** - 像素级实例分割，支持掩膜多边形
- **图像分类** - Top-K 分类结果与置信度分数
- **关键点检测** - 姿态估计和关键点检测
- **多环境支持** - 自动检测和切换 Conda/virtualenv 环境
- **模型管理** - 按任务类型组织的模型存储

### 图像处理
- **基础操作**：灰度化、反色、二值化
- **变换操作**：旋转（左/右90°）、翻转（水平/垂直）
- **滤镜效果**：模糊、锐化、边缘检测（Sobel、Canny、Laplacian）
- **参数调整**：亮度、对比度、饱和度

### 文件管理
- **多标签页支持** - 同时编辑多张图片
- **文件树浏览器** - 左侧目录导航面板
- **最近文件** - 快速访问最近打开的文件
- **拖放支持** - 支持文件和文件夹拖放
- **导出功能** - 支持多种格式导出（JSON、XML、CSV、图片）

### 批量处理
- **文件夹批量处理** - 处理整个目录
- **进度跟踪** - 实时进度显示，支持取消
- **结果汇总** - 统计信息和详细日志

### 用户界面
- **启动画面** - 带初始化进度的加载界面
- **撤销/重做** - 每标签页50步历史记录
- **剪贴板操作** - 复制/粘贴图片
- **键盘快捷键** - 高效工作流程快捷键
- **实时日志** - 操作日志输出面板

## 快速开始

### 环境要求
- Windows 10/11
- Visual Studio 2022
- Qt 6.9.3 (msvc2022_64)
- CMake 3.16+
- Python 3.8+ with Conda（用于 YOLO 推理）

### Python 环境配置

创建并配置用于 YOLO 推理的 Conda 环境：

```bash
# 创建环境
conda create -n GenPreCVSystem python=3.10
conda activate GenPreCVSystem

# 安装 ultralytics
pip install ultralytics

# 或使用环境扫描器自动检测现有环境
```

### 构建

```bash
# 使用 CMake 预设配置
cmake --preset msvc-release

# 构建
cmake --build build --config Release
```

### 运行

```bash
# 运行应用程序
./build/Release/GenPreCVSystem.exe
```

### 部署

```batch
deploy.bat
```

可执行文件位于：`dist/GenPreCVSystem/GenPreCVSystem.exe`

## 项目结构

```
GenPreCVSystem/
├── src/                           # 源代码目录
│   ├── core/                      # 应用核心
│   │   ├── main.cpp               # 程序入口
│   │   ├── splashscreen.*         # 启动画面
│   │   └── application.h          # 应用设置
│   ├── config/                    # 配置管理
│   │   ├── appsettings.*          # 应用设置
│   │   ├── recentfilesmanager.*   # 最近文件管理
│   │   └── tasktypes.h            # 任务类型定义
│   ├── models/                    # 数据模型
│   │   ├── tabdata.h              # 标签页数据模型
│   │   ├── undostack.h            # 撤销/重做栈
│   │   └── pythonenvironment.h    # Python 环境模型
│   ├── services/                  # 业务逻辑服务
│   │   ├── environment/           # Python 环境管理
│   │   │   ├── environmentcachemanager.*  # 环境缓存
│   │   │   └── environmentscanner.*       # 环境检测
│   │   ├── image/                 # 图像处理服务
│   │   │   ├── imageprocessor.*   # 基础图像操作
│   │   │   └── imageprocessservice.*  # 图像服务外观
│   │   ├── inference/             # 模型推理服务
│   │   │   ├── dlservice.*        # DL Python 后端服务
│   │   │   └── python/            # Python 脚本
│   │   │       └── dl_service.py      # DL 后端脚本
│   │   ├── io/                    # 文件 I/O 服务
│   │   │   ├── fileutils.*        # 文件工具
│   │   │   └── exportservice.*    # 导出功能
│   │   └── system/                # 系统服务
│   │       ├── clipboardhelper.*  # 剪贴板操作
│   │       ├── exceptions.*       # 异常处理
│   │       └── errordialog.*      # 错误对话框
│   ├── views/                     # UI 视图
│   │   ├── mainwindow.*           # 主窗口
│   │   ├── imageview.*            # 图像查看组件
│   │   ├── imagefilefilterproxy.* # 文件过滤
│   │   ├── dialogs/               # 对话框窗口
│   │   │   ├── batchprocessdialog.*   # 批量处理
│   │   │   ├── detectionresultdialog.* # 检测结果
│   │   │   └── settingsdialog.*   # 应用设置
│   │   └── components/            # 可复用 UI 组件
│   │       ├── filetreeview.*     # 文件树控件
│   │       └── environmentservicewidget.* # 环境选择器
│   ├── controllers/               # MVC 控制器
│   │   ├── tabcontroller.*        # 标签页管理
│   │   ├── imagecontroller.*      # 图像操作
│   │   ├── filecontroller.*       # 文件操作
│   │   ├── taskcontroller.*       # 任务执行
│   │   └── parameterpanelfactory.* # 参数 UI 工厂
│   └── resources/                 # 资源文件
│       ├── models/                # 模型文件（git忽略）
│       │   ├── classification/    # 分类模型
│       │   ├── detection/         # 检测模型
│       │   ├── segmentation/      # 分割模型
│       │   └── keypoint/          # 关键点模型
│       └── *.ts                   # 翻译文件
├── tests/                         # 单元和集成测试
├── docs/                          # 文档
├── third_party/                   # 第三方库
├── build/                         # 构建目录
├── dist/                          # 分发输出
├── CMakeLists.txt                 # CMake 配置
└── deploy.bat                     # 部署脚本
```

## 快捷键

| 功能 | 快捷键 |
|------|--------|
| 打开图片 | `Ctrl+O` |
| 打开文件夹 | `Ctrl+D` |
| 关闭标签 | `Ctrl+W` |
| 保存 | `Ctrl+S` |
| 另存为 | `Ctrl+Shift+S` |
| 复制 | `Ctrl+C` |
| 粘贴 | `Ctrl+V` |
| 向左旋转 | `Ctrl+L` |
| 向右旋转 | `Ctrl+R` |
| 水平翻转 | `Ctrl+H` |
| 垂直翻转 | `Ctrl+Shift+H` |
| 放大 | `Ctrl++` |
| 缩小 | `Ctrl+-` |
| 适应窗口 | `Ctrl+F` |
| 实际大小 | `Ctrl+1` |
| 运行任务 | `F5` |
| 停止任务 | `Shift+F5` |
| 设置 | `Ctrl+,` |
| 撤销 | `Ctrl+Z` |
| 重做 | `Ctrl+Y` |

## 使用说明

### 打开图片
1. **菜单方式**：文件 → 打开图片 (`Ctrl+O`)
2. **文件夹方式**：文件 → 打开文件夹 (`Ctrl+D`)，然后双击图片
3. **拖放方式**：直接拖入文件或文件夹到文件浏览器
4. **最近文件**：从 文件 → 最近文件 菜单访问

### YOLO 推理
1. **选择 Python 环境**：使用右侧面板的环境选择器
2. **选择任务类型**：选择检测/分割/分类/关键点
3. **加载模型**：选择模型文件 (.pt 或 .onnx)
4. **调整参数**：设置置信度阈值、IoU 阈值、图像尺寸
5. **运行**：点击运行 (`F5`) 或按运行按钮
6. **查看结果**：检测结果以边界框/掩膜形式显示
7. **导出**：保存结果为 JSON、XML、CSV 或标注图片

### 批量处理
1. **打开批量对话框**：工具 → 批量处理
2. **选择输入文件夹**：选择包含图片的目录
3. **配置设置**：选择模型和参数
4. **开始处理**：点击开始并监控进度
5. **查看结果**：查看汇总并导出

### 管理设置
1. **打开设置**：文件 → 设置 (`Ctrl+,`)
2. **配置选项**：
   - 常规：语言、主题、自动保存
   - 处理：默认阈值、输出格式
   - Python：默认环境路径
   - 快捷键：自定义键盘快捷键
3. **保存**：设置自动持久化

## YOLO 服务架构

YOLO 推理服务使用 Qt C++ 前端与 Python 后端架构：

```
┌─────────────────┐     QProcess      ┌──────────────────┐
│  DLService      │ ◄─── (JSON IPC) ─►│   dl_service.py  │
│   (C++/Qt)      │                   │  (Python/Ultralytics)
└─────────────────┘                   └──────────────────┘
         │                                       │
         ▼                                       ▼
┌─────────────────┐                      ┌──────────────────┐
│   Qt 信号       │                      │   YOLO 模型      │
│  (UI 更新)      │                      │  (PyTorch/onnx)  │
└─────────────────┘                      └──────────────────┘
```

### 特性：
- **环境自动发现**：自动扫描 Conda/virtualenv 环境
- **缓存机制**：环境验证结果缓存，实现快速启动
- **异步操作**：非阻塞环境扫描
- **错误处理**：优雅降级和详细错误信息

### 示例：使用 DLService

```cpp
#include "services/inference/dlservice.h"

using namespace GenPreCVSystem::Utils;

DLService* service = new DLService(this);

// 扫描可用的 Python 环境
QVector<PythonEnvironment> envs = service->scanEnvironments();

// 设置环境并启动服务
service->setEnvironmentPath("/path/to/python.exe");
if (service->start()) {
    // 加载模型
    if (service->loadModel("yolov8n.pt")) {
        // 运行检测
        YOLODetectionResult result = service->detect(
            "image.jpg",
            0.25f,  // 置信度阈值
            0.45f,  // IoU 阈值
            640     // 图像尺寸
        );

        if (result.success) {
            for (const auto& det : result.detections) {
                qDebug() << "检测到:" << det.label
                         << "置信度:" << det.confidence;
            }
        }
    }
}
```

## 支持的 YOLO 模型

- **检测**：YOLOv8n/s/m/l/x、YOLOv9、YOLOv10、YOLOv11、YOLOv26
- **分割**：YOLOv8n-seg、YOLOv8s-seg 等
- **分类**：YOLOv8n-cls、YOLOv8s-cls 等
- **姿态/关键点**：YOLOv8n-pose、YOLOv8s-pose 等

模型按任务类型组织在 `src/resources/models/` 目录下。

## 技术栈

| 组件 | 技术 |
|------|------|
| 框架 | Qt 6.9.3 |
| 语言 | C++17 |
| 构建系统 | CMake 3.16+ |
| 编译器 | MSVC 2022 |
| Python 后端 | Ultralytics YOLO |
| 架构模式 | MVC |

## 测试

运行单元和集成测试：

```bash
# 启用测试构建
cmake --preset msvc-release -DBUILD_TESTS=ON
cmake --build build --config Release

# 运行测试
ctest --test-dir build -C Release
```

## 贡献指南

欢迎提交 Issue 和 Pull Request！

## 许可证

Copyright © 2024-2025 GenPreCVSystem. All rights reserved.

</details>

---

<div align="center">

**Made with ❤️ by GenPreCVSystem Team**

</div>
