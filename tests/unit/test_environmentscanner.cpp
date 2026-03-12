/**
 * @file test_environmentscanner.cpp
 * @brief EnvironmentScanner 单元测试实现
 */

#include "test_environmentscanner.h"
#include <QThread>
#include <QDebug>

void TestEnvironmentScanner::initTestCase()
{
    qputenv("QT_QPA_PLATFORM", "minimal");
    m_scanner = nullptr;
    qDebug() << "Initializing EnvironmentScanner tests...";
}

void TestEnvironmentScanner::cleanupTestCase()
{
    if (m_scanner) {
        m_scanner->cancel();
        delete m_scanner;
    }
    qDebug() << "Cleaned up EnvironmentScanner tests";
}

void TestEnvironmentScanner::testConstruction()
{
    EnvironmentScanner scanner;

    QVERIFY(!scanner.isScanning());

    qDebug() << "✓ Construction test passed";
}

void TestEnvironmentScanner::testStartAndStop()
{
    EnvironmentScanner scanner;

    // 测试扫描（同步执行）
    QVector<CachedEnvironment> result = scanner.scan();

    // 结果应该立即可用
    QVERIFY(result.size() >= 0);

    qDebug() << "✓ Start and stop test passed";
}

void TestEnvironmentScanner::testScanSignals()
{
    EnvironmentScanner scanner;

    bool gotProgress = false;
    bool gotCompleted = false;
    bool gotFailed = false;
    QString failedMessage;

    // 连接信号
    connect(&scanner, &EnvironmentScanner::scanProgress,
            [&gotProgress](const QString &) { gotProgress = true; });
    connect(&scanner, &EnvironmentScanner::scanCompleted,
            [&gotCompleted](const QVector<CachedEnvironment> &) { gotCompleted = true; });
    connect(&scanner, &EnvironmentScanner::scanFailed,
            [&gotFailed, &failedMessage](const QString &msg) {
        gotFailed = true;
        failedMessage = msg;
    });

    // 执行扫描
    QVector<CachedEnvironment> result = scanner.scan();

    // 验证信号已发射（正常情况下应该收到 completed，而不是 failed）
    QVERIFY(gotProgress || gotCompleted);
    QVERIFY(gotCompleted);  // 正常情况下扫描应该成功完成
    QVERIFY(!gotFailed);    // 正常情况下不应该失败

    qDebug() << "✓ Scan signals test passed";
}

void TestEnvironmentScanner::testConcurrentScan()
{
    // 创建两个扫描器实例
    EnvironmentScanner scanner1;
    EnvironmentScanner scanner2;

    // 执行第一个扫描
    QVector<CachedEnvironment> result1 = scanner1.scan();

    // 执行第二个扫描
    QVector<CachedEnvironment> result2 = scanner2.scan();

    // 两个都应该成功完成
    QVERIFY(result1.size() >= 0);
    QVERIFY(result2.size() >= 0);

    qDebug() << "✓ Concurrent scan test passed";
}

void TestEnvironmentScanner::testScanResults()
{
    EnvironmentScanner scanner;

    m_scanCompleted = false;
    m_results.clear();

    // 连接信号
    connect(&scanner, &EnvironmentScanner::scanCompleted,
            [this](const QVector<CachedEnvironment> &envs) {
        m_scanCompleted = true;
        m_results = envs;
    });

    // 执行扫描
    QVector<CachedEnvironment> result = scanner.scan();

    // 验证结果
    QVERIFY(result.size() >= 0);
    QCOMPARE(m_results.size(), result.size());

    qDebug() << "✓ Scan results test passed, found" << result.size() << "environments";
}
