# 贡献指南

感谢您考虑为 GenPreCVSystem 做出贡献！

## 目录

- [行为准则](#行为准则)
- [如何贡献](#如何贡献)
- [开发流程](#开发流程)
- [代码规范](#代码规范)
- [提交规范](#提交规范)
- [审核流程](#审核流程)

---

## 行为准则

参与本项目即表示您同意：
- 尊重所有参与者
- 接受建设性批评
- 关注对社区最有利的事情
- 对其他社区成员表示同理心

## 如何贡献

### 报告 Bug

如果您发现了 Bug，请提交 Issue 并包含：

1. **问题描述** - 清晰简洁的描述
2. **复现步骤** - 详细步骤说明
3. **期望行为** - 您期望发生什么
4. **实际行为** - 实际发生了什么
5. **截图** - 如有必要
6. **环境信息**：
   - 操作系统版本
   - 软件版本
   - Qt 版本
   - Python 版本
   - 显卡型号（如相关）

### 建议新功能

提交 Feature Request 时：
1. 清晰描述功能
2. 说明使用场景
3. 如果可能，提供示例代码或 UI 设计

### 提交代码

1. Fork 本仓库
2. 创建功能分支：`git checkout -b feature/AmazingFeature`
3. 提交更改：`git commit -m 'Add some AmazingFeature'`
4. 推送分支：`git push origin feature/AmazingFeature`
5. 创建 Pull Request

---

## 开发流程

### 环境搭建

```bash
# 1. Fork 并克隆仓库
git clone https://github.com/YOUR_USERNAME/GenPreCVSystem.git
cd GenPreCVSystem

# 2. 添加上游仓库
git remote add upstream https://github.com/jinweixia03/GenPreCVSystem.git

# 3. 创建构建目录
cmake --preset msvc-release

# 4. 构建
cmake --build build --config Release
```

### 分支策略

- `master` - 稳定版本分支
- `develop` - 开发分支
- `feature/*` - 功能分支
- `hotfix/*` - 紧急修复分支
- `release/*` - 发布分支

### 工作流程

```bash
# 1. 同步上游代码
git checkout master
git pull upstream master
git push origin master

# 2. 创建功能分支
git checkout -b feature/my-feature

# 3. 开发并提交
git add .
git commit -m "feat: add my feature"

# 4. 保持分支同步
git fetch upstream
git rebase upstream/master

# 5. 推送并创建 PR
git push origin feature/my-feature
```

---

## 代码规范

### C++ 规范

#### 命名规范

```cpp
// 类名 - PascalCase
class ImageProcessor { };
class DLService { };

// 函数名 - camelCase
void processImage();
QString getFileName();

// 变量名 - camelCase
int maxImageSize;
QString modelPath;

// 成员变量 - m_ 前缀
class MyClass {
    int m_privateMember;
    QString m_modelPath;
};

// 常量 - k 前缀 + PascalCase
const int kMaxRecentFiles = 10;
const float kDefaultConfThreshold = 0.25f;

// 宏 - 全大写 + 下划线
#define DEBUG_MODE 1
```

#### 代码格式

```cpp
// 缩进 - 4 个空格
// 括号风格 - Allman 风格
void function()
{
    if (condition) {
        doSomething();
    } else {
        doOtherThing();
    }
}

// 行长度 - 不超过 100 字符
// 空行 - 函数之间、逻辑块之间
```

#### 头文件规范

```cpp
#ifndef FILENAME_H
#define FILENAME_H

// 标准库头文件
#include <vector>
#include <string>

// Qt 头文件
#include <QString>
#include <QImage>

// 项目头文件
#include "otherheader.h"

namespace GenPreCVSystem {
namespace Module {

class MyClass
{
public:
    explicit MyClass(QObject* parent = nullptr);
    ~MyClass();

    // 公共接口
    void publicMethod();

signals:
    void somethingHappened();

private:
    void privateMethod();
    int m_privateMember;
};

} // namespace Module
} // namespace GenPreCVSystem

#endif // FILENAME_H
```

#### 文档注释

```cpp
/**
 * @brief 类/函数的简要描述
 * @param paramName 参数说明
 * @return 返回值说明
 * @note 注意事项
 * @example 使用示例
 */
```

### Python 规范

遵循 PEP 8：

```python
# 命名
class MyService:          # 类名 - PascalCase
def my_function():        # 函数 - snake_case
my_variable = 1           # 变量 - snake_case
CONSTANT_VALUE = 100      # 常量 - UPPER_SNAKE_CASE

# 缩进 - 4 个空格
# 行长度 - 100 字符
# 文档字符串 - Google Style

def process_image(image_path: str, conf: float = 0.25) -> dict:
    """处理图像并返回结果。

    Args:
        image_path: 图像文件路径
        conf: 置信度阈值

    Returns:
        包含检测结果的字典

    Raises:
        FileNotFoundError: 图像文件不存在
    """
    pass
```

### CMake 规范

```cmake
# 最小版本要求
cmake_minimum_required(VERSION 3.16)

# 项目名称
project(MyProject VERSION 1.0.0 LANGUAGES CXX)

# 变量命名 - 全大写
set(SOURCE_FILES
    src/main.cpp
    src/utils.cpp
)

# 目标命名 - 小写
target_sources(my_target PRIVATE ${SOURCE_FILES})
```

---

## 提交规范

使用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

### 格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type

| 类型 | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档更新 |
| `style` | 代码格式（不影响功能） |
| `refactor` | 代码重构 |
| `perf` | 性能优化 |
| `test` | 测试相关 |
| `chore` | 构建/工具相关 |

### Scope

可选，表示影响范围：
- `ui` - 界面相关
- `core` - 核心功能
- `inference` - 推理服务
- `image` - 图像处理
- `docs` - 文档

### 示例

```bash
# 新功能
feat(inference): add YOLOv11 model support

# Bug 修复
fix(ui): resolve memory leak in image view

# 文档
docs: update API documentation

# 性能优化
perf(image): optimize grayscale conversion

# 代码重构
refactor(core): simplify settings dialog

# 范围 + 描述 + 正文
feat(batch): add progress dialog for batch processing

- Add progress bar with cancel button
- Show real-time processing status
- Export results on completion

Fixes #123
```

---

## 审核流程

### PR 要求

1. **标题清晰** - 使用提交规范格式
2. **描述详细** - 说明做了什么、为什么做
3. **关联 Issue** - 如有相关 Issue 请引用
4. **通过测试** - 确保代码可编译、可运行
5. **文档更新** - 如需要请更新 README/文档

### 审核检查清单

- [ ] 代码符合规范
- [ ] 有适当的注释
- [ ] 无内存泄漏
- [ ] 无明显的性能问题
- [ ] 向后兼容（除非特别说明）

### 合并策略

- 使用 **Squash and Merge**
- 确保提交信息清晰
- 删除已合并的功能分支

---

## 开发提示

### 调试技巧

```cpp
// 使用 Qt 的调试输出
qDebug() << "Debug message:" << variable;
qWarning() << "Warning message";
qCritical() << "Critical error";

// 条件断点
if (condition) {
    qDebug() << "Breakpoint here";
}
```

### 性能分析

```bash
# Windows 性能分析
# 使用 Visual Studio Profiler 或 Intel VTune

# Python 性能分析
python -m cProfile -o output.prof script.py
```

### 内存检查

```bash
# 使用 Address Sanitizer
cmake -B build -S . -DENABLE_ASAN=ON

# 使用 Valgrind (Linux)
valgrind --leak-check=full ./program
```

---

## 社区

- GitHub Issues: [问题反馈](https://github.com/jinweixia03/GenPreCVSystem/issues)
- Discussions: [技术讨论](https://github.com/jinweixia03/GenPreCVSystem/discussions)

---

感谢您对 GenPreCVSystem 的贡献！
