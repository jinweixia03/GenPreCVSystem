#ifndef ENVIRONMENTSCANNER_H
#define ENVIRONMENTSCANNER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <functional>
#include "dlservice.h"
#include "environmentcachemanager.h"

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 环境扫描器
 *
 * 使用简单的同步扫描，避免复杂的线程和事件循环问题。
 * 扫描在调用线程中执行，UI 层负责在后台线程调用。
 */
class EnvironmentScanner : public QObject
{
    Q_OBJECT

public:
    explicit EnvironmentScanner(QObject *parent = nullptr);
    ~EnvironmentScanner();

    /**
     * @brief 执行扫描（同步，会阻塞直到完成）
     * @return 扫描到的环境列表
     */
    QVector<CachedEnvironment> scan();

    /**
     * @brief 取消扫描
     */
    void cancel();

    /**
     * @brief 检查是否正在扫描
     */
    bool isScanning() const { return m_isScanning; }

signals:
    /**
     * @brief 扫描进度信号
     */
    void scanProgress(const QString &message);

    /**
     * @brief 扫描完成信号
     * @param environments 扫描到的环境列表
     */
    void scanCompleted(const QVector<CachedEnvironment> &environments);

    /**
     * @brief 扫描失败信号
     * @param errorMessage 错误信息
     */
    void scanFailed(const QString &errorMessage);

private:
    QVector<PythonEnvironment> doScan();

    volatile bool m_shouldStop;
    volatile bool m_isScanning;
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // ENVIRONMENTSCANNER_H
