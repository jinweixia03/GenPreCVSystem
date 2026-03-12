/**
 * @file test_main.cpp
 * @brief GenPreCVSystem 测试主入口
 *
 * 使用 Qt Test 框架进行单元测试和集成测试
 */

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <iostream>

// 包含所有测试类
#include "unit/test_environmentcachemanager.h"
#include "unit/test_yoloservice.h"
#include "unit/test_environmentscanner.h"
#include "integration/test_environmentworkflow.h"

int main(int argc, char *argv[])
{
    // 使用 QCoreApplication 而不是 QApplication，因为不需要 GUI
    QCoreApplication app(argc, argv);

    std::cout << "========================================" << std::endl;
    std::cout << "GenPreCVSystem Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    int result = 0;
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;

    // 运行环境缓存管理器测试
    std::cout << "\n[1/4] EnvironmentCacheManager Tests:" << std::endl;
    std::cout.flush();
    {
        TestEnvironmentCacheManager cacheTest;
        result = QTest::qExec(&cacheTest, argc, argv);
        totalTests += cacheTest.testCount();
        if (result == 0) {
            passedTests += cacheTest.testCount();
            std::cout << "✓ EnvironmentCacheManager tests passed" << std::endl;
        } else {
            failedTests += cacheTest.testCount();
            std::cout << "✗ EnvironmentCacheManager tests failed" << std::endl;
        }
    }
    // {
    //     TestEnvironmentCacheManager cacheTest;
    //     result = QTest::qExec(&cacheTest, argc, argv);
    //     totalTests += cacheTest.testCount();
    //     if (result == 0) {
    //         passedTests += cacheTest.testCount();
    //         std::cout << "✓ EnvironmentCacheManager tests passed" << std::endl;
    //     } else {
    //         failedTests += cacheTest.testCount();
    //         std::cout << "✗ EnvironmentCacheManager tests failed" << std::endl;
    //     }
    // }

    // 运行 YOLOService 测试
    std::cout << "\n[2/4] YOLOService Tests:" << std::endl;
    std::cout.flush();
    {
        TestYOLOService yoloTest;
        result = QTest::qExec(&yoloTest, argc, argv);
        totalTests += yoloTest.testCount();
        if (result == 0) {
            passedTests += yoloTest.testCount();
            std::cout << "✓ YOLOService tests passed" << std::endl;
        } else {
            failedTests += yoloTest.testCount();
            std::cout << "✗ YOLOService tests failed" << std::endl;
        }
    }

    // 运行 EnvironmentScanner 测试
    std::cout << "\n[3/4] EnvironmentScanner Tests:" << std::endl;
    std::cout.flush();
    {
        TestEnvironmentScanner scannerTest;
        result = QTest::qExec(&scannerTest, argc, argv);
        totalTests += scannerTest.testCount();
        if (result == 0) {
            passedTests += scannerTest.testCount();
            std::cout << "✓ EnvironmentScanner tests passed" << std::endl;
        } else {
            failedTests += scannerTest.testCount();
            std::cout << "✗ EnvironmentScanner tests failed" << std::endl;
        }
    }

    // 运行集成测试
    std::cout << "\n[4/4] Integration Tests:" << std::endl;
    std::cout.flush();
    {
        TestEnvironmentWorkflow workflowTest;
        result = QTest::qExec(&workflowTest, argc, argv);
        totalTests += workflowTest.testCount();
        if (result == 0) {
            passedTests += workflowTest.testCount();
            std::cout << "✓ Integration tests passed" << std::endl;
        } else {
            failedTests += workflowTest.testCount();
            std::cout << "✗ Integration tests failed" << std::endl;
        }
    }

    // 输出测试摘要
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Summary:" << std::endl;
    std::cout << "  Total:  " << totalTests << std::endl;
    std::cout << "  Passed: " << passedTests << std::endl;
    std::cout << "  Failed: " << failedTests << std::endl;
    std::cout << "  Success Rate:" << (totalTests > 0 ? (passedTests * 100 / totalTests) : 0) << "%" << std::endl;
    std::cout << "========================================" << std::endl;

    return failedTests > 0 ? 1 : 0;
}
