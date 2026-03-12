#ifndef TEST_ENVIRONMENTWORKFLOW_H
#define TEST_ENVIRONMENTWORKFLOW_H

#include <QObject>
#include <QtTest/QtTest>
#include "../../src/utils/yoloservice.h"
#include "../../src/utils/environmentcachemanager.h"
#include "../../src/utils/environmentscanner.h"

using namespace GenPreCVSystem::Utils;

/**
 * @brief 环境工作流集成测试
 *
 * 测试缓存管理器、扫描器和 YOLO 服务之间的协作
 */
class TestEnvironmentWorkflow : public QObject
{
    Q_OBJECT

public:
    int testCount() const { return 4; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 集成测试
    void testCacheAndScanIntegration();
    void testFullWorkflow();
    void testEnvironmentSwitching();
    void testServiceLifecycle();
};

#endif // TEST_ENVIRONMENTWORKFLOW_H
