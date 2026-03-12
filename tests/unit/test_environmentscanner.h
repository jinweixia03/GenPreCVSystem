#ifndef TEST_ENVIRONMENTSCANNER_H
#define TEST_ENVIRONMENTSCANNER_H

#include <QObject>
#include <QtTest/QtTest>
#include "../../src/utils/environmentscanner.h"

using namespace GenPreCVSystem::Utils;

/**
 * @brief EnvironmentScanner 单元测试
 */
class TestEnvironmentScanner : public QObject
{
    Q_OBJECT

public:
    int testCount() const { return 5; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 基本功能测试
    void testConstruction();
    void testStartAndStop();
    void testScanSignals();
    void testConcurrentScan();
    void testScanResults();

private:
    EnvironmentScanner *m_scanner;
    bool m_scanCompleted;
    QVector<CachedEnvironment> m_results;
};

#endif // TEST_ENVIRONMENTSCANNER_H
