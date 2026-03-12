/**
 * @file test_environmentcachemanager.cpp
 * @brief EnvironmentCacheManager 单元测试实现
 */

#include "test_environmentcachemanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <iostream>

void TestEnvironmentCacheManager::initTestCase()
{
    // 确保使用测试目录
    qputenv("QT_QPA_PLATFORM", "minimal");
    qDebug() << "Initializing EnvironmentCacheManager tests...";

    // 确保没有正在运行的后台验证
    EnvironmentCacheManager *cacheMgr = EnvironmentCacheManager::instance();
    if (cacheMgr->isBackgroundValidationRunning()) {
        cacheMgr->stopBackgroundValidation();
    }
}

void TestEnvironmentCacheManager::cleanupTestCase()
{
    // 停止后台验证，避免测试结束时卡住
    EnvironmentCacheManager *cacheMgr = EnvironmentCacheManager::instance();
    if (cacheMgr->isBackgroundValidationRunning()) {
        cacheMgr->stopBackgroundValidation();
    }

    // 清理测试数据
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QFile::remove(cacheDir + "/environment_cache_v1.json");
    QFile::remove(cacheDir + "/environment_state.json");
    qDebug() << "Cleaned up test data";
}

void TestEnvironmentCacheManager::testSingleton()
{
    qDebug() << "Starting singleton test...";
    std::cout << "Starting singleton test..." << std::endl;
    // 测试单例模式
    EnvironmentCacheManager *instance1 = EnvironmentCacheManager::instance();
    qDebug() << "Got instance1:" << (instance1 != nullptr);
    std::cout << "Got instance1: " << (instance1 != nullptr) << std::endl;
    EnvironmentCacheManager *instance2 = EnvironmentCacheManager::instance();
    qDebug() << "Got instance2:" << (instance2 != nullptr);
    std::cout << "Got instance2: " << (instance2 != nullptr) << std::endl;

    QVERIFY(instance1 != nullptr);
    QVERIFY(instance1 == instance2);
    qDebug() << "✓ Singleton test passed";
    std::cout << "Singleton test passed" << std::endl;
}

void TestEnvironmentCacheManager::testInitialize()
{
    std::cout << "Starting initialize test..." << std::endl;
    EnvironmentCacheManager *cacheMgr = EnvironmentCacheManager::instance();
    std::cout << "Got cache manager instance" << std::endl;

    // 先清空缓存，避免后台验证干扰测试
    std::cout << "Clearing cache..." << std::endl;
    cacheMgr->clearCache();
    std::cout << "Cache cleared" << std::endl;

    // 初始化应该成功（不启动后台验证，避免测试卡住）
    std::cout << "Initializing with false..." << std::endl;
    cacheMgr->initialize(false);
    std::cout << "Initialized" << std::endl;

    // 获取统计信息
    std::cout << "Getting stats..." << std::endl;
    EnvironmentCacheStats stats = cacheMgr->getStats();
    std::cout << "Got stats, total: " << stats.totalEnvironments << std::endl;
    QVERIFY(stats.totalEnvironments >= 0);

    std::cout << "Initialize test passed" << std::endl;
    qDebug() << "✓ Initialize test passed";
}

void TestEnvironmentCacheManager::testAddAndRetrieveCache()
{
    EnvironmentCacheManager *cacheMgr = EnvironmentCacheManager::instance();

    // 创建测试环境
    CachedEnvironment env;
    env.name = "TestEnv";
    env.path = "/test/python";
    env.type = "conda";
    env.isValid = true;
    env.hasUltralytics = true;
    env.hasTorch = true;
    env.pythonVersion = "3.9.0";

    // 添加到缓存
    cacheMgr->updateCache(env);

    // 验证可以获取
    CachedEnvironment retrieved = cacheMgr->getEnvironment("/test/python");
    QCOMPARE(retrieved.name, QString("TestEnv"));
    QCOMPARE(retrieved.path, QString("/test/python"));
    QVERIFY(retrieved.hasUltralytics);

    qDebug() << "✓ Add and retrieve cache test passed";
}

void TestEnvironmentCacheManager::testUpdateCache()
{
    EnvironmentCacheManager *cacheMgr = EnvironmentCacheManager::instance();

    // 创建并添加环境
    CachedEnvironment env;
    env.name = "UpdateTest";
    env.path = "/update/test";
    env.type = "venv";
    env.hasUltralytics = false;

    cacheMgr->updateCache(env);

    // 更新环境
    env.hasUltralytics = true;
    env.pythonVersion = "3.10.0";
    cacheMgr->updateCache(env);

    // 验证更新
    CachedEnvironment updated = cacheMgr->getEnvironment("/update/test");
    QVERIFY(updated.hasUltralytics);
    QCOMPARE(updated.pythonVersion, QString("3.10.0"));

    qDebug() << "✓ Update cache test passed";
}

void TestEnvironmentCacheManager::testClearCache()
{
    EnvironmentCacheManager *cacheMgr = EnvironmentCacheManager::instance();

    // 添加测试数据
    CachedEnvironment env;
    env.name = "ClearTest";
    env.path = "/clear/test";
    cacheMgr->updateCache(env);

    QVERIFY(cacheMgr->getCachedEnvironments().size() > 0);

    // 清空缓存
    cacheMgr->clearCache();

    // 验证缓存为空
    QVERIFY(cacheMgr->getCachedEnvironments().isEmpty());

    qDebug() << "✓ Clear cache test passed";
}

void TestEnvironmentCacheManager::testCacheExpiration()
{
    CachedEnvironment env;
    env.name = "ExpireTest";
    env.path = "/expire/test";
    env.cacheUpdatedAt = QDateTime::currentDateTime().addDays(-2);

    // 应该过期（默认24小时）
    QVERIFY(env.isExpired());

    // 不应该过期（设置7天）
    QVERIFY(!env.isExpired(168));

    qDebug() << "✓ Cache expiration test passed";
}

void TestEnvironmentCacheManager::testLastUsedEnvironment()
{
    EnvironmentCacheManager *cacheMgr = EnvironmentCacheManager::instance();

    // 设置最后使用的环境
    cacheMgr->setLastUsedEnvironment("/last/used/env");

    // 验证可以获取
    QString lastUsed = cacheMgr->getLastUsedEnvironment();
    QCOMPARE(lastUsed, QString("/last/used/env"));

    // 设置最后使用的模型
    cacheMgr->setLastUsedModel("/path/to/model.pt");
    QCOMPARE(cacheMgr->getLastUsedModel(), QString("/path/to/model.pt"));

    qDebug() << "✓ Last used environment test passed";
}
