#ifndef TEST_ENVIRONMENTCACHEMANAGER_H
#define TEST_ENVIRONMENTCACHEMANAGER_H

#include <QObject>
#include <QtTest/QtTest>
#include "../../src/utils/environmentcachemanager.h"

using namespace GenPreCVSystem::Utils;

/**
 * @brief EnvironmentCacheManager 单元测试
 */
class TestEnvironmentCacheManager : public QObject
{
    Q_OBJECT

public:
    int testCount() const { return 8; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 基本功能测试
    void testSingleton();
    void testInitialize();
    void testAddAndRetrieveCache();
    void testUpdateCache();
    void testClearCache();
    void testCacheExpiration();
    void testLastUsedEnvironment();
};

#endif // TEST_ENVIRONMENTCACHEMANAGER_H
