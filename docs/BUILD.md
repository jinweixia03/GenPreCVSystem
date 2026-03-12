# 构建指南

本文档详细介绍如何从源码构建 GenPreCVSystem。

## 目录

- [环境要求](#环境要求)
- [Windows 构建](#windows-构建)
- [常见问题](#常见问题)

---

## 环境要求

### 必需组件

| 组件 | 版本 | 下载链接 |
|------|------|----------|
| Windows SDK | 10.0.19041+ | 随 VS 安装 |
| Visual Studio | 2022 | [下载](https://visualstudio.microsoft.com/) |
| Qt | 6.9.3 (msvc2022_64) | [下载](https://www.qt.io/download) |
| CMake | 3.16+ | [下载](https://cmake.org/download/) |
| Python | 3.8+ | [下载](https://www.python.org/downloads/) |

### Visual Studio 工作负载

安装时需勾选以下工作负载：
- **使用 C++ 的桌面开发**
- **Windows 11 SDK** (或 Windows 10 SDK)

### Qt 组件选择

在 Qt Maintenance Tool 中安装：
- Qt 6.9.3
  - MSVC 2022 64-bit
  - Qt 5 Compatibility Module
  - Qt Image Formats
  - Qt SVG
  - Qt Translations
- Developer and Designer Tools
  - Qt Creator 13.0.0
  - CMake 3.27+ (可选，也可单独安装)

---

## Windows 构建

### 1. 克隆仓库

```bash
git clone https://github.com/jinweixia03/GenPreCVSystem.git
cd GenPreCVSystem
```

### 2. 配置环境变量

确保 Qt 和 CMake 在 PATH 中：

```powershell
# PowerShell
$env:PATH = "C:\Qt\6.9.3\msvc2022_64\bin;$env:PATH"
$env:PATH = "C:\Program Files\CMake\bin;$env:PATH"
```

或在系统环境变量中永久添加：
- `C:\Qt\6.9.3\msvc2022_64\bin`
- `C:\Program Files\CMake\bin`

### 3. 配置项目

#### 方式一：使用 CMake 预设（推荐）

```bash
cmake --preset msvc-release
```

#### 方式二：手动配置

```bash
cmake -B build -S . ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DQt6_DIR="C:/Qt/6.9.3/msvc2022_64/lib/cmake/Qt6" ^
  -DCMAKE_BUILD_TYPE=Release
```

### 4. 构建项目

```bash
# 构建 Release 版本
cmake --build build --config Release --parallel

# 或构建 Debug 版本
cmake --build build --config Debug --parallel
```

### 5. 运行程序

```bash
# 运行 Release 版本
.\build\Release\GenPreCVSystem.exe

# 或运行 Debug 版本
.\build\Debug\GenPreCVSystem.exe
```

---

## 使用 Qt Creator

### 1. 打开项目

1. 启动 Qt Creator
2. 选择 **文件 → 打开文件或项目**
3. 选择 `CMakeLists.txt`

### 2. 配置 Kits

1. 选择 **项目 → 管理 Kits**
2. 确保已配置 Desktop Qt 6.9.3 MSVC2022 64bit
3. 选择 Release 或 Debug 构建配置

### 3. 构建运行

点击左下角 ▶️ 按钮或按 `Ctrl+R`

---

## 部署

### 使用部署脚本

```bash
deploy.bat
```

部署后的文件位于 `dist/GenPreCVSystem/`

### 手动部署

```bash
# 创建部署目录
mkdir dist\GenPreCVSystem

# 复制可执行文件
copy build\Release\GenPreCVSystem.exe dist\GenPreCVSystem\

# 复制 Qt 依赖
windeployqt dist\GenPreCVSystem\GenPreCVSystem.exe

# 复制 Python 服务脚本
mkdir dist\GenPreCVSystem\services
xcopy src\services\inference\python dist\GenPreCVSystem\services\ /E /I
```

---

## 常见问题

### CMake 找不到 Qt

**错误信息：**
```
CMake Error: Could not find a package configuration file provided by "Qt6"
```

**解决方案：**
```bash
cmake -B build -S . -DQt6_DIR="C:/Qt/6.9.3/msvc2022_64/lib/cmake/Qt6"
```

### 编译器版本不匹配

**错误信息：**
```
error: C1189: #error:  "Qt requires a C++17 compiler"
```

**解决方案：**
确保使用 Visual Studio 2022 或更新版本，并在 CMakeLists.txt 中添加：
```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Python 环境检测失败

**问题：** 软件启动后无法找到 Python

**解决方案：**
1. 确保 Python 已添加到系统 PATH
2. 或设置环境变量：
   ```powershell
   $env:PYTHON_PATH = "C:\Python310\python.exe"
   ```

### 运行时缺少 DLL

**错误信息：**
```
无法启动此程序，因为计算机中丢失 Qt6Core.dll
```

**解决方案：**
1. 将 Qt 的 bin 目录添加到 PATH
2. 或运行 `windeployqt` 复制依赖 DLL

---

## 高级配置

### 启用测试

```bash
cmake -B build -S . -DBUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release
```

### 静态链接 Qt

```bash
cmake -B build -S . -DQT_STATIC=ON
```

### 自定义安装路径

```bash
cmake -B build -S . -DCMAKE_INSTALL_PREFIX="C:/Program Files/GenPreCVSystem"
cmake --build build --config Release
cmake --install build
```

---

## 获取帮助

如遇到构建问题，请：

1. 检查 [FAQ.md](FAQ.md)
2. 提交 [Issue](https://github.com/jinweixia03/GenPreCVSystem/issues)
3. 提供以下信息：
   - 操作系统版本
   - Visual Studio 版本
   - Qt 版本
   - CMake 版本
   - 完整的错误日志
