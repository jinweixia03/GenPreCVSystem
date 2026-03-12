# 常见问题 (FAQ)

本文档整理了使用 GenPreCVSystem 时的常见问题及解决方案。

---

## 安装与启动

### Q: 软件启动时提示缺少 DLL 文件？

**A:** 这是 Qt 运行时库未找到导致的。

**解决方案：**
1. 确保已安装 Qt 6.9.3 (msvc2022_64)
2. 将 Qt 的 bin 目录添加到系统 PATH：
   ```
   C:\Qt\6.9.3\msvc2022_64\bin
   ```
3. 或使用 `windeployqt` 工具复制依赖 DLL 到程序目录

### Q: 启动时提示 "找不到 Python 环境"？

**A:** 软件需要 Python 3.8+ 环境进行深度学习推理。

**解决方案：**
1. 安装 Python 3.8 或更高版本
2. 安装时勾选 "Add Python to PATH"
3. 或安装 Anaconda/Miniconda
4. 重启软件后点击「扫描环境」按钮

### Q: 扫描不到 Conda 环境？

**A:** Conda 可能未正确初始化。

**解决方案：**
1. 在命令行运行 `conda init`
2. 确保 Conda 在系统 PATH 中
3. 手动添加 Conda 安装路径到软件设置

---

## Python 环境

### Q: 如何选择正确的 Python 环境？

**A:** 在右侧面板的环境选择器中：
1. 选择带 ✓ 标记的环境（表示已安装 ultralytics）
2. 优先选择支持 CUDA 的环境（NVIDIA 显卡）
3. 推荐使用 Conda 虚拟环境

### Q: 安装 ultralytics 失败？

**A:** 可能是网络或依赖问题。

**解决方案：**
```bash
# 使用清华镜像加速
pip install ultralytics -i https://pypi.tuna.tsinghua.edu.cn/simple

# 或先升级 pip
python -m pip install --upgrade pip
pip install ultralytics
```

### Q: 如何验证 Python 环境是否正常？

**A:** 在命令行执行：
```bash
python -c "import ultralytics; import torch; print('OK')"
```

如果无错误输出，则环境正常。

---

## 模型相关

### Q: 支持哪些模型格式？

**A:** 当前支持：
- `.pt` - PyTorch 模型（推荐）
- `.pth` - PyTorch 状态字典
- `.onnx` - ONNX 格式（实验性支持）

### Q: 如何获取 YOLO 模型？

**A:**
1. **官方预训练模型：**
   ```bash
   pip install ultralytics
   yolo download yolov8n.pt
   ```

2. **Ultralytics 文档：**
   https://docs.ultralytics.com/models/

3. **自定义训练模型：**
   使用 YOLOv8 训练自己的数据集

### Q: 模型加载失败？

**A:** 可能原因及解决方案：

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| "模型文件不存在" | 路径错误 | 检查文件路径 |
| "模型格式不支持" | 非 YOLO 模型 | 使用 YOLOv8/v9/v10/v11 模型 |
| "模型加载超时" | 模型过大 | 等待更长时间或换用小模型 |
| "CUDA out of memory" | 显存不足 | 换用 CPU 推理或减小 image_size |

### Q: 可以使用自己的训练模型吗？

**A:** 可以。确保：
1. 使用 YOLOv8/v9/v10/v11 训练
2. 导出为 `.pt` 格式
3. 类别标签与模型匹配
4. 选择正确的任务类型（检测/分割/分类/关键点）

---

## 推理问题

### Q: 推理速度很慢？

**A:** 优化建议：

1. **使用 GPU 加速**
   - 确保 NVIDIA 显卡驱动已安装
   - 安装 CUDA 版本的 PyTorch
   - 在环境选择器中选择 CUDA 环境

2. **调整参数**
   - 减小 `image_size`（如从 640 改为 320）
   - 使用更小的模型（如 yolov8n 代替 yolov8x）

3. **批量处理**
   - 使用批量处理功能充分利用 GPU

### Q: 检测结果不准确？

**A:** 调整以下参数：

1. **置信度阈值 (conf_threshold)**
   - 降低阈值（如 0.25 → 0.15）增加检出率
   - 提高阈值（如 0.25 → 0.5）减少误检

2. **IoU 阈值 (iou_threshold)**
   - 降低阈值减少重叠框
   - 提高阈值允许更多重叠

3. **图像尺寸 (image_size)**
   - 增大尺寸提高小目标检测精度
   - 注意：会消耗更多显存

### Q: 批量处理时程序崩溃？

**A:** 可能原因：
1. **内存不足** - 减小 batch size
2. **图片损坏** - 检查输入图片是否完整
3. **路径过长** - Windows 路径限制 260 字符

---

## GPU/CUDA 问题

### Q: 如何检查是否在使用 GPU？

**A:**
1. 在 Python 环境中运行：
   ```bash
   python -c "import torch; print(torch.cuda.is_available())"
   ```
   输出 `True` 表示可用

2. 在软件中查看环境选择器，带 🚀 图标表示支持 CUDA

### Q: CUDA out of memory 错误？

**A:** 显存不足，解决方案：
1. 减小 `image_size` 参数
2. 关闭其他占用显存的程序
3. 使用 CPU 推理
4. 升级显卡

### Q: 安装了 CUDA 但检测不到？

**A:**
1. 检查 NVIDIA 驱动版本（需要 450.80.02+）
2. 确保 PyTorch 是 CUDA 版本：
   ```bash
   pip install torch torchvision --index-url https://download.pytorch.org/whl/cu118
   ```
3. 检查 CUDA 版本兼容性

---

## 图像处理

### Q: 支持哪些图片格式？

**A:** 支持格式：
- 标准格式：PNG, JPG, JPEG, BMP, GIF
- 高级格式：TIFF, WEBP, HEIC
- 支持透明通道（PNG）

### Q: 如何处理大分辨率图片？

**A:**
1. 使用「适应窗口」查看完整图片
2. 推理时调整 `image_size` 参数
3. 批量处理时会自动缩放

### Q: 撤销/重做不工作？

**A:**
- 每个标签页独立维护 50 步历史记录
- 切换标签页后历史记录保持不变
- 关闭标签页后历史记录清除

---

## 导出与文件

### Q: 导出格式有什么区别？

**A:**

| 格式 | 用途 | 内容 |
|------|------|------|
| JSON | 程序读取 | 完整结构化数据 |
| CSV | 表格分析 | 简化表格数据 |
| XML | 兼容性好 | 标准标记格式 |
| 图片 | 可视化 | 带标注的图像 |

### Q: 批量处理结果在哪里？

**A:** 默认保存在：
```
[导出目录]/[任务类型]_[时间戳]/
├── results.json      # 检测结果
├── annotated/        # 标注图片
└── summary.csv       # 汇总统计
```

### Q: 如何修改默认导出目录？

**A:** 文件 → 设置 → 默认导出目录

---

## 性能优化

### Q: 如何提高批量处理速度？

**A:**
1. 使用 GPU 加速
2. 减小 image_size
3. 提高置信度阈值（减少输出）
4. 关闭实时预览

### Q: 软件占用内存过大？

**A:**
1. 关闭不需要的标签页
2. 清理最近文件列表
3. 重启软件释放内存

---

## 其他问题

### Q: 如何报告 Bug？

**A:** 提交 Issue 时包含：
1. 操作系统版本
2. 软件版本
3. 复现步骤
4. 错误截图或日志
5. 相关文件（如模型、图片）

### Q: 有使用教程吗？

**A:** 查看：
- [README.md](../README.md) - 快速开始
- 软件内帮助菜单（F1）
- 快捷键：Ctrl+, 打开设置

### Q: 如何更新软件？

**A:**
1. 备份自定义模型和设置
2. 拉取最新代码：`git pull`
3. 重新编译：`cmake --build build --config Release`
4. 或下载最新发布版本

---

## 获取帮助

如果以上问题未能解决您的疑问：

1. 查看 [API 文档](API.md)
2. 查看 [构建指南](BUILD.md)
3. 提交 [GitHub Issue](https://github.com/jinweixia03/GenPreCVSystem/issues)
4. 联系开发团队

---

*最后更新：2025-03-13*
