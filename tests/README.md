# GenPreCVSystem 测试套件

## 概述

本文档描述了 GenPreCVSystem 的测试框架和使用方法。

## 测试结构

```
tests/
├── test_main.cpp                    # 测试主入口
├── CMakeLists.txt                   # 测试构建配置
├── run_tests.bat                    # Windows 测试运行脚本
├── README.md                        # 本文档
├── unit/                            # 单元测试
│   ├── test_environmentcachemanager.h/cpp
│   ├── test_yoloservice.h/cpp
│   └── test_environmentscanner.h/cpp
└── integration/                     # 集成测试
    └── test_environmentworkflow.h/cpp
```

## 运行测试

### 方法一：使用 CMake 构建（推荐）

```bash
# 配置并构建（启用测试）
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --parallel 4

# 运行测试
ctest -V
```

### 方法二：使用测试脚本

```bash
cd tests
run_tests.bat
```

### 方法三：手动运行

```bash
# 构建后运行测试可执行文件
build\Debug\GenPreCVSystemTests.exe
```

## 测试覆盖

### 单元测试

1. **EnvironmentCacheManager Tests**
   - 单例模式测试
   - 初始化和缓存操作
   - 缓存过期机制
   - 最后使用环境/模型记录

2. **YOLOService Tests**
   - 构造和状态管理
   - 环境路径设置
   - 环境扫描功能
   - 快速启动检查
   - 模型操作

3. **EnvironmentScanner Tests**
   - 线程生命周期
   - 信号发射
   - 并发扫描处理
   - 扫描结果验证

### 集成测试

1. **EnvironmentWorkflow Tests**
   - 缓存与扫描集成
   - 完整工作流程
   - 环境切换
   - 服务生命周期

## 添加新测试

1. 在 `unit/` 或 `integration/` 目录创建测试头文件和实现文件
2. 继承 `QObject` 并使用 `Q_OBJECT` 宏
3. 在 `private slots:` 中定义测试函数
4. 在 `test_main.cpp` 中包含并运行测试

### 示例

```cpp
#ifndef TEST_MYFEATURE_H
#define TEST_MYFEATURE_H

#include <QObject>
#include <QtTest/QtTest>

class TestMyFeature : public QObject
{
    Q_OBJECT

public:
    int testCount() const { return 3; }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCase1();
    void testCase2();
    void testCase3();
};

#endif
```

## 测试原则

1. **独立性**：每个测试用例应该独立运行，不依赖其他测试
2. **可重复性**：测试应该总是产生相同的结果
3. **快速性**：测试应该快速执行
4. **清晰性**：测试名称应该清楚地说明测试内容

## 调试测试

使用 Qt Creator:
1. 打开项目
2. 选择 GenPreCVSystemTests 目标
3. 设置断点
4. 启动调试

命令行调试:
```bash
# 详细输出
GenPreCVSystemTests.exe -v2

# 只运行特定测试
GenPreCVSystemTests.exe testCase1
```
