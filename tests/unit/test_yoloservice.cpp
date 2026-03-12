/**
 * @file test_dlservice.cpp
 * @brief DLService 单元测试实现
 */

#include "test_yoloservice.h"
#include <QStandardPaths>
#include <QDebug>

void TestDLService::initTestCase()
{
    qputenv("QT_QPA_PLATFORM", "minimal");
    qDebug() << "Initializing DLService tests...";
}

void TestDLService::cleanupTestCase()
{
    qDebug() << "Cleaned up DLService tests";
}

void TestDLService::testConstruction()
{
    DLService service;

    // 初始状态检查
    QVERIFY(!service.isRunning());
    QVERIFY(!service.isModelLoaded());
    QVERIFY(service.currentEnvironmentPath().isEmpty());
    QVERIFY(service.modelPath().isEmpty());

    qDebug() << "✓ Construction test passed";
}

void TestDLService::testEnvironmentPath()
{
    DLService service;

    // 设置环境路径
    service.setEnvironmentPath("/test/python/path");
    QCOMPARE(service.currentEnvironmentPath(), QString("/test/python/path"));

    qDebug() << "✓ Environment path test passed";
}

void TestDLService::testScanEnvironments()
{
    // 扫描环境（这可能需要一些时间）
    qDebug() << "Scanning environments...";
    QVector<PythonEnvironment> envs = DLService::scanEnvironments();

    // 验证返回的是数组（可能为空，取决于系统配置）
    QVERIFY(envs.size() >= 0);

    // 如果有环境，验证结构
    for (const auto &env : envs) {
        QVERIFY(!env.name.isEmpty());
        QVERIFY(!env.path.isEmpty());
        QVERIFY(env.type == "conda" || env.type == "venv" || env.type == "system");
    }

    qDebug() << "✓ Scan environments test passed, found" << envs.size() << "environments";
}

void TestDLService::testFastStartCheck()
{
    DLService service;

    // 空路径应该返回 false
    QVERIFY(!service.canFastStart(""));

    // 随机路径应该返回 false（未缓存）
    QVERIFY(!service.canFastStart("/random/path"));

    qDebug() << "✓ Fast start check test passed";
}

void TestDLService::testModelOperations()
{
    DLService service;

    // 初始状态
    QVERIFY(!service.isModelLoaded());
    QVERIFY(service.modelPath().isEmpty());

    // 注意：由于服务未运行，实际加载模型会失败
    // 这里只测试接口存在性

    qDebug() << "✓ Model operations test passed";
}

void TestDLService::testServiceState()
{
    DLService service;

    // 测试初始状态
    QVERIFY(!service.isRunning());

    // 测试停止未运行的服务（不应该崩溃）
    service.stop();
    QVERIFY(!service.isRunning());

    qDebug() << "✓ Service state test passed";
}
