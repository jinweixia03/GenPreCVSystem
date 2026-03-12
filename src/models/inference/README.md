# 模型推理目录

## 状态

**此目录当前为空。**

## 历史记录

此目录原本包含基于 LibTorch (PyTorch C++ API) 的原生 C++ 模型推理实现：

- `modelinference.h` - LibTorch 推理基类接口
- `modelinference.cpp` - LibTorch 推理实现

## 移除原因

1. **依赖复杂** - LibTorch 库体积较大 (~1GB)，增加部署复杂度
2. **维护成本** - 需要单独维护 C++ 推理逻辑和 Python 模型转换
3. **功能重复** - Python 后端（Ultralytics YOLO）已满足所有推理需求
4. **开发效率** - Python 后端更易于调试和更新模型

## 当前架构

项目现在使用 **C++/Qt + Python 后端** 混合架构：

```
┌─────────────────┐     QProcess      ┌──────────────────┐
│   DLService     │ ◄─── (JSON IPC) ─►│   dl_service.py  │
│   (C++/Qt)      │                   │  (Python/YOLO)   │
└─────────────────┘                   └──────────────────┘
         │                                       │
         ▼                                       ▼
┌─────────────────┐                      ┌──────────────────┐
│   Qt Signals    │                      │  Ultralytics     │
│  (UI Updates)   │                      │    Models        │
└─────────────────┘                      └──────────────────┘
```

## 如需恢复 LibTorch

如需恢复 LibTorch 支持，可从 Git 历史中提取相关文件：

```bash
# 查看历史提交找到相关文件
git log --all --full-history -- src/models/inference/modelinference.*

# 恢复文件
git checkout <commit> -- src/models/inference/modelinference.h
git checkout <commit> -- src/models/inference/modelinference.cpp
```

然后需要在 `CMakeLists.txt` 中添加 LibTorch 配置。

## 参考

- [PyTorch C++ API 文档](https://pytorch.org/cppdocs/)
- [TorchScript 教程](https://pytorch.org/tutorials/advanced/cpp_export.html)
