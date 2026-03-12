#ifndef TEST_DLSERVICE_H
#define TEST_DLSERVICE_H

#include <QObject>
#include <QtTest/QtTest>
#include "services/inference/dlservice.h"

using namespace GenPreCVSystem::Utils;

/**
 * @brief DLService 单元测试
 */
class TestDLService : public QObject
{
    Q_OBJECT

public:
    int testCount() const { return 6; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 基本功能测试
    void testConstruction();
    void testEnvironmentPath();
    void testScanEnvironments();
    void testFastStartCheck();
    void testModelOperations();
    void testServiceState();
};

#endif // TEST_DLSERVICE_H
