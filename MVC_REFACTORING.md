# MVC架构重构总结

## 重构概述

原 `mainwindow.cpp` (3127行) 已按照MVC架构重构为以下模块化结构：

## 新的目录结构

```
src/
├── models/              # 数据模型层
│   ├── tabdata.h                 # 标签页数据模型
│   ├── tasktypes.h               # 任务类型枚举
│   └── undostack.h               # 撤销/重做栈模型
│
├── views/               # 视图层
│   ├── imageview.h/.cpp          # 图片显示视图
│   ├── filetreeview.h/.cpp       # 文件树视图
│   └── imagefilefilterproxy.h/.cpp # 文件过滤代理模型
│
├── controllers/         # 控制器层
│   ├── tabcontroller.h/.cpp      # 标签页管理
│   ├── imagecontroller.h/.cpp    # 图像处理控制
│   ├── filecontroller.h/.cpp     # 文件操作控制
│   ├── taskcontroller.h/.cpp     # 任务切换控制
│   └── parameterpanelfactory.h/.cpp # 参数面板工厂
│
└── utils/               # 工具类层
    ├── imageprocessor.h/.cpp     # 图像处理算法
    ├── clipboardhelper.h/.cpp    # 剪贴板操作
    └── fileutils.h/.cpp          # 文件操作工具
```

## 各模块职责

### Models (数据模型)

| 类名 | 文件 | 职责 |
|------|------|------|
| `TabData` | tabdata.h | 标签页数据结构 |
| `CVTask` | tasktypes.h | CV任务类型枚举 |
| `UndoStack` | undostack.h | 撤销/重做栈管理 |

### Views (视图组件)

| 类名 | 文件 | 职责 |
|------|------|------|
| `ImageView` | imageview.h/cpp | 图片显示、缩放、平移 |
| `FileTreeView` | filetreeview.h/cpp | 文件树显示、拖放 |
| `ImageFileFilterProxyModel` | imagefilefilterproxy.h/cpp | 图片文件过滤 |

### Controllers (控制器)

| 类名 | 文件 | 职责 |
|------|------|------|
| `TabController` | tabcontroller.h/cpp | 标签页增删、切换、状态管理 |
| `ImageController` | imagecontroller.h/cpp | 图像滤镜、变换操作 |
| `FileController` | filecontroller.h/cpp | 文件保存、复制、删除、重命名 |
| `TaskController` | taskcontroller.h/cpp | 任务切换、参数面板更新 |
| `ParameterPanelFactory` | parameterpanelfactory.h/cpp | 创建任务参数面板UI |

### Utils (工具类)

| 类名 | 文件 | 职责 |
|------|------|------|
| `ImageProcessor` | imageprocessor.h/cpp | 图像处理算法实现 |
| `ClipboardHelper` | clipboardhelper.h/cpp | 剪贴板操作 |
| `FileUtils` | fileutils.h/cpp | 文件操作工具函数 |

## 重构后的MainWindow

重构后的MainWindow应该只负责：
1. UI组件的组装和布局
2. 创建各个Controller实例
3. 连接信号槽
4. 协调各Controller之间的交互

### 使用示例

```cpp
// 在MainWindow构造函数中
void MainWindow::setupControllers()
{
    // 创建控制器
    m_tabController = new Controllers::TabController(tabWidget, this);
    m_imageController = new Controllers::ImageController(this);
    m_fileController = new Controllers::FileController(this, this);
    m_taskController = new Controllers::TaskController(this);

    // 连接信号
    connect(m_tabController, &Controllers::TabController::imageLoaded,
            this, &MainWindow::onImageLoaded);
    connect(m_imageController, &Controllers::ImageController::imageChanged,
            this, &MainWindow::onImageChanged);

    // 设置参数面板
    m_taskController->setParameterScrollArea(paramScrollArea);
}
```

## 迁移指南

### 1. 原有的ImageView类
```cpp
// 原代码 (嵌入在mainwindow.h中)
class ImageView : public QGraphicsView { ... };

// 新代码
using ImageView = GenPreCVSystem::Views::ImageView;
```

### 2. 原有的FileTreeView类
```cpp
// 原代码
class FileTreeView : public QTreeView { ... };

// 新代码
using FileTreeView = GenPreCVSystem::Views::FileTreeView;
```

### 3. 图像处理操作
```cpp
// 原代码 (直接在MainWindow中实现)
void MainWindow::on_actionGrayscale_triggered()
{
    QImage image = m_currentPixmap.toImage();
    // ... 处理逻辑
    m_currentPixmap = QPixmap::fromImage(image);
    currentImageView()->setPixmap(m_currentPixmap);
}

// 新代码 (使用ImageController)
void MainWindow::on_actionGrayscale_triggered()
{
    m_imageController->setCurrentImage(m_tabController->currentPixmap(),
                                       m_tabController->currentUndoStack());
    m_imageController->toGrayscale();
}
```

### 4. 文件操作
```cpp
// 原代码
QString fileName = QFileDialog::getOpenFileName(this, "打开图片", "", filter);
if (!fileName.isEmpty()) {
    // ... 处理逻辑
}

// 新代码 (使用FileController)
QString fileName = m_fileController->openImage();
if (!fileName.isEmpty()) {
    m_tabController->loadImage(fileName);
}
```

## 编译说明

### 前置要求
- Qt 5.15+ 或 Qt 6.x
- CMake 3.16+
- C++17兼容的编译器

### 编译步骤
```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/version/compiler
cmake --build . --config Release
```

### 配置Qt路径
如果CMake找不到Qt，请设置 `CMAKE_PREFIX_PATH`：
```bash
# Windows (Qt6)
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"

# Linux
cmake .. -DCMAKE_PREFIX_PATH="/opt/Qt6.x.x/gcc_64"

# macOS
cmake .. -DCMAKE_PREFIX_PATH="/usr/local/Qt6.x.x"
```

## 优势

1. **代码组织更清晰**：每个模块职责单一，易于理解
2. **便于维护**：修改某个功能只需关注对应的Controller
3. **便于测试**：各模块可独立测试
4. **便于扩展**：添加新功能只需添加新的Controller或扩展现有Controller
5. **复用性高**：Utils层的类可以在其他项目中复用

## 后续工作

1. 修改 `mainwindow.cpp` 使用新的MVC组件
2. 更新UI信号槽连接
3. 编译测试验证功能完整性
4. 逐步迁移现有功能到对应的Controller

## 注意事项

- 所有新创建的类都在 `GenPreCVSystem` 命名空间下
- 使用时需要包含相应的头文件
- 部分Controller的信号需要连接到MainWindow的槽函数
