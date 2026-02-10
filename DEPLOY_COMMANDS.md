# GenPreCVSystem 部署命令序列

## 方法一：使用部署脚本（推荐）

直接运行项目根目录下的部署脚本：

```batch
deploy.bat
```

部署脚本会自动完成以下操作：
1. 检查可执行文件和windeployqt工具
2. 创建 `dist/GenPreCVSystem` 目录
3. 复制可执行文件到分发目录
4. 运行 windeployqt 部署Qt依赖
5. 清理根目录中的临时文件

## 方法二：手动执行命令序列

### 1. 设置环境变量

```batch
set PROJECT_DIR=D:\1_Source\Code\GenPreCVSystem
set BUILD_DIR=%PROJECT_DIR%\build
set RELEASE_DIR=%BUILD_DIR%\Release
set DIST_DIR=%PROJECT_DIR%\dist\GenPreCVSystem
set QT_PATH=D:\Environments\CPlusPlus\Qt\6.9.3\msvc2022_64
```

### 2. 创建分发目录并复制可执行文件

```batch
if exist %DIST_DIR% rmdir /s /q %DIST_DIR%
mkdir %DIST_DIR%
copy %RELEASE_DIR%\GenPreCVSystem.exe %DIST_DIR%\
```

### 3. 运行 windeployqt 部署

```batch
"%QT_PATH%\bin\windeployqt.exe" --dir %DIST_DIR% %RELEASE_DIR%\GenPreCVSystem.exe
```

### 4. 验证部署结果

```batch
dir %DIST_DIR%
```

## 快速一键部署命令

```batch
"D:\Environments\CPlusPlus\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe" --dir dist/GenPreCVSystem build/Release/GenPreCVSystem.exe
```

## 项目目录结构

```
GenPreCVSystem/
├── .gitignore              # Git忽略文件
├── CMakeLists.txt          # CMake配置
├── CMakePresets.json       # CMake预设
├── deploy.bat              # 部署脚本
├── src/                    # 源代码目录
│   ├── main.cpp
│   ├── mainwindow.h
│   ├── mainwindow.cpp
│   ├── mainwindow.ui
│   └── GenPreCVSystem_en_AS.ts
├── build/                  # CMake构建目录
│   └── Release/
│       └── GenPreCVSystem.exe
├── dist/                   # 分发目录
│   └── GenPreCVSystem/
│       ├── GenPreCVSystem.exe      # 主程序
│       ├── Qt6Core.dll             # Qt 核心库
│       ├── Qt6Gui.dll              # Qt GUI 库
│       ├── Qt6Widgets.dll          # Qt Widgets 库
│       ├── Qt6Network.dll          # Qt 网络库
│       ├── Qt6Svg.dll              # SVG 支持
│       ├── platforms/              # 平台插件
│       │   └── qwindows.dll
│       ├── imageformats/           # 图像格式支持
│       │   ├── qjpeg.dll
│       │   ├── qpng.dll
│       │   └── ...
│       ├── iconengines/            # 图标引擎
│       ├── styles/                 # 样式插件
│       ├── translations/           # Qt 翻译文件
│       └── ...                     # 其他运行时文件
└── .vscode/                # VSCode设置
```

## 注意事项

1. **Qt路径**：确保 Qt 路径正确，如果 Qt 安装位置不同，请修改 `QT_PATH` 变量
2. **构建顺序**：部署前需要先构建项目（Release 模式）
3. **独立运行**：`dist/GenPreCVSystem` 目录可以独立运行，无需安装 Qt
4. **版本控制**：`dist/` 目录已在 `.gitignore` 中，不会被提交到Git
5. **清理**：部署脚本会自动清理根目录中的临时Qt文件

## 运行程序

部署完成后，可以直接运行：

```batch
dist\GenPreCVSystem\GenPreCVSystem.exe
```

或者双击 `dist\GenPreCVSystem\GenPreCVSystem.exe` 文件。
