# GenPreCVSystem

一个工业级计算机视觉预处理系统，基于 Qt 6.9 和 C++17 开发。

## 功能特性

- 📋 **任务管理**：8种CV任务类型，带专属参数配置面板
  - 图像分类 (Image Classification) - ResNet, EfficientNet, ViT等模型
  - 目标检测 (Object Detection) - YOLO, Faster R-CNN等模型
  - 语义分割 (Semantic Segmentation) - DeepLab, U-Net等模型
  - 实例分割 (Instance Segmentation) - Mask R-CNN等模型
  - 关键点检测 (Key Point Detection) - MMPose, OpenPose等
  - 图像增强 (Image Enhancement) - 亮度、对比度、饱和度调整
  - 图像去噪 (Image Denoising) - 高斯滤波、双边滤波等
  - 边缘检测 (Edge Detection) - Sobel, Canny, Laplacian等
- 📑 **多标签页**：支持同时打开多张图片，每张图片独立编辑历史
- 🖱️ **拖放支持**：可从外部拖入图片文件或文件夹到文件浏览器
- 📋 **剪贴板**：复制/粘贴图片到剪贴板（同时支持图片数据和文件引用）
- ↩️ **撤销/重做**：最多50步操作历史，每个标签页独立管理
- ⚙️ **参数面板**：针对不同CV任务的专属参数配置界面
- 📁 **文件浏览器**：左侧文件树视图，支持导航、右键菜单、图片文件过滤
- 🖼️ **图片查看器**：中央工作区，支持缩放、平移、适应窗口等功能
- 🎨 **图像处理**：
  - 基础操作：灰度化、反色、二值化
  - 变换操作：旋转（左/右90°）、翻转（水平/垂直）
  - 滤镜效果：模糊、锐化、边缘检测
- 📝 **日志输出**：底部实时日志显示，记录所有操作

## 快速开始

### 构建项目

```bash
# 使用 CMake 预设构建
cmake --preset msvc-release
cmake --build build --config Release
```

### 部署应用

```batch
# 运行部署脚本
deploy.bat
```

部署完成后，可执行文件位于：`dist/GenPreCVSystem/GenPreCVSystem.exe`

## 项目结构

```
GenPreCVSystem/
├── src/                           # 源代码目录
│   ├── main.cpp                  # 主程序入口
│   ├── mainwindow.h              # 主窗口头文件
│   ├── mainwindow.cpp            # 主窗口实现
│   ├── mainwindow.ui             # UI界面文件
│   ├── GenPreCVSystem_en_AS.ts   # 英文翻译文件
│   └── model/                    # 模型相关目录（预留）
├── build/                         # CMake构建目录
├── dist/                          # 分发目录（部署输出）
├── .vscode/                       # VSCode配置
│   ├── settings.json             # VSCode设置（UTF-8编码）
│   └── c_cpp_properties.json     # C++ IntelliSense配置
├── CMakeLists.txt                 # CMake配置
├── CMakePresets.json              # CMake预设配置
├── deploy.bat                     # Windows部署脚本
├── DEPLOY_COMMANDS.md             # 部署命令说明
└── README.md                      # 项目说明文档
```

## 快捷键

| 功能 | 快捷键 |
|------|--------|
| 打开图片 | Ctrl+O |
| 打开文件夹 | Ctrl+D |
| 关闭图片 | Ctrl+W |
| 保存图片 | Ctrl+S |
| 另存为 | Ctrl+Shift+S |
| 复制图片 | Ctrl+C |
| 粘贴图片 | Ctrl+V |
| 向左旋转 | Ctrl+L |
| 向右旋转 | Ctrl+R |
| 水平翻转 | Ctrl+H |
| 垂直翻转 | Ctrl+Shift+H |
| 放大 | Ctrl++ |
| 缩小 | Ctrl+- |
| 适应窗口 | Ctrl+F |
| 实际大小 | Ctrl+1 |
| 运行处理 | F5 |
| 停止处理 | Shift+F5 |
| 设置 | Ctrl+, |
| 帮助 | F1 |
| 撤销 | Ctrl+Z |
| 重做 | Ctrl+Y |
| 图像分类 | Ctrl+Shift+C |
| 目标检测 | Ctrl+Shift+O |
| 语义分割 | Ctrl+Shift+S |
| 实例分割 | Ctrl+Shift+I |
| 关键点检测 | Ctrl+Shift+K |
| 图像增强 | Ctrl+Shift+E |
| 图像去噪 | Ctrl+Shift+D |
| 边缘检测 | Ctrl+Shift+G |

## 使用说明

### 打开图片

1. **菜单方式**：文件 → 打开图片 (Ctrl+O)
2. **文件夹方式**：文件 → 打开文件夹 (Ctrl+D)，然后双击文件树中的图片
3. **拖放方式**：直接将图片文件或文件夹拖入文件浏览器

### 编辑图片

1. **基本编辑**：使用"编辑"菜单或快捷键进行旋转、翻转
2. **图像处理**：使用"图像"菜单应用滤镜效果（灰度化、模糊、锐化等）
3. **撤销/重做**：Ctrl+Z 撤销，Ctrl+Y 重做（最多50步历史）

### CV任务处理

1. **选择任务**：从"任务"菜单选择CV任务类型
2. **配置参数**：在右侧参数面板调整任务相关参数
   - 每个任务类型都有专属的参数配置界面
   - 参数包括模型选择、训练参数、推理参数等
3. **运行处理**：点击"运行处理" (F5)

### 文件管理

- **右键菜单**：在文件浏览器中右键可执行：
  - 打开图片/文件夹
  - 在资源管理器中显示
  - 复制文件路径/图片
  - 重命名/删除
  - 粘贴图片
- **向上导航**：点击导航栏的"↑"按钮返回上级目录
- **新建文件夹**：在空白处右键 → 新建文件夹

### 保存导出

- **保存**：覆盖原文件 (Ctrl+S)，会提示选择"覆盖"或"保存副本"
- **另存为**：保存到新文件 (Ctrl+Shift+S)
- **导出**：导出处理后的图片，自动生成文件名

## 技术栈

- **Qt 6.9.3**：跨平台应用程序框架
- **C++17**：现代C++标准
- **CMake**：构建系统
- **MSVC 2022**：编译器

## 开发环境

- Windows 10/11
- Visual Studio 2022
- Qt 6.9.3 (msvc2022_64)
- CMake 3.16+
- VSCode (可选)

## 许可证

Copyright © 2024 GenPreCVSystem. All rights reserved.
