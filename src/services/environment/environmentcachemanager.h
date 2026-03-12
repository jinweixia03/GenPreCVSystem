#ifndef ENVIRONMENTCACHEMANAGER_H
#define ENVIRONMENTCACHEMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QReadWriteLock>
#include <QMutex>
#include "pythonenvironment.h"

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 缓存的环境信息（扩展版本）
 *
 * 包含完整的验证状态和缓存元数据
 */
struct CachedEnvironment : public PythonEnvironment {
    bool isValid = false;           // 环境是否有效（路径存在）
    bool hasTorch = false;          // 是否安装了 PyTorch
    bool hasOpenCV = false;         // 是否安装了 OpenCV
    bool hasNumPy = false;          // 是否安装了 NumPy
    QString pythonVersion;          // Python 版本号
    QString ultralyticsVersion;     // ultralytics 版本号
    QString torchVersion;           // torch 版本号
    QDateTime validatedAt;          // 上次验证时间
    QDateTime cacheUpdatedAt;       // 缓存更新时间
    int validationTimeMs = 0;       // 验证耗时（毫秒）

    // GPU 信息
    bool cudaAvailable = false;     // CUDA 是否可用
    int cudaDeviceCount = 0;        // CUDA 设备数量
    QString cudaVersion;            // CUDA 版本
    QString gpuName;                // GPU 名称
    QString gpuMemory;              // GPU 显存信息

    CachedEnvironment() = default;
    explicit CachedEnvironment(const PythonEnvironment &env);

    /**
     * @brief 检查缓存是否过期
     * @param maxAgeHours 最大缓存时间（小时）
     * @return 是否过期
     */
    bool isExpired(int maxAgeHours = 24) const;

    /**
     * @brief 检查缓存是否需要重新验证
     * @return 是否需要验证
     */
    bool needsRevalidation() const;

    /**
     * @brief 获取 GPU 状态摘要
     * @return GPU 状态字符串（如 "NVIDIA RTX 3060 (12GB)" 或 "CPU only"）
     */
    QString getGpuStatusString() const;

    /**
     * @brief 检查是否有可用的 GPU
     * @return 是否有可用 GPU
     */
    bool hasGpu() const { return cudaAvailable && cudaDeviceCount > 0; }
};

/**
 * @brief 环境缓存统计信息
 */
struct EnvironmentCacheStats {
    int totalEnvironments = 0;      // 总环境数
    int validEnvironments = 0;      // 有效环境数
    int readyEnvironments = 0;      // 就绪环境数（有 ultralytics）
    int expiredEnvironments = 0;    // 过期环境数
    QDateTime lastFullScan;         // 上次完整扫描时间
    QDateTime lastQuickScan;        // 上次快速扫描时间
    int averageValidationTimeMs = 0; // 平均验证时间
};

/**
 * @brief Python 环境缓存管理器
 *
 * 提供高性能的环境状态缓存：
 * - 缓存环境验证结果，避免重复验证
 * - 支持快速启动（使用缓存数据）
 * - 后台异步验证更新缓存
 * - 持久化缓存到磁盘
 */
class EnvironmentCacheManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static EnvironmentCacheManager* instance();

    /**
     * @brief 初始化缓存管理器（加载磁盘缓存）
     * @param enableBackgroundValidation 是否启动后台验证，默认为 true
     */
    void initialize(bool enableBackgroundValidation = true);

    /**
     * @brief 获取所有缓存的环境（快速，不验证）
     * @return 环境列表
     */
    QVector<CachedEnvironment> getCachedEnvironments() const;

    /**
     * @brief 获取就绪的环境（有 ultralytics）
     * @return 就绪环境列表
     */
    QVector<CachedEnvironment> getReadyEnvironments() const;

    /**
     * @brief 根据路径获取缓存的环境
     * @param path Python 可执行文件路径
     * @return 环境信息（未找到返回空）
     */
    CachedEnvironment getEnvironment(const QString &path) const;

    /**
     * @brief 检查环境是否已缓存且有效
     * @param path Python 可执行文件路径
     * @return 是否有效
     */
    bool isEnvironmentValid(const QString &path) const;

    /**
     * @brief 检查环境是否已就绪（已验证有 ultralytics）
     * @param path Python 可执行文件路径
     * @return 是否就绪
     */
    bool isEnvironmentReady(const QString &path) const;

    /**
     * @brief 快速验证环境（使用缓存，必要时异步更新）
     * @param path Python 可执行文件路径
     * @return 验证结果
     */
    CachedEnvironment quickValidate(const QString &path);

    /**
     * @brief 完整验证环境（同步，更新缓存）
     * @param path Python 可执行文件路径
     * @return 验证结果
     */
    CachedEnvironment fullValidate(const QString &path);

    /**
     * @brief 更新或添加环境到缓存
     * @param env 环境信息
     */
    void updateCache(const CachedEnvironment &env);

    /**
     * @brief 批量更新缓存
     * @param envs 环境列表
     */
    void updateCacheBatch(const QVector<CachedEnvironment> &envs);

    /**
     * @brief 从缓存中移除环境
     * @param path Python 可执行文件路径
     */
    void removeFromCache(const QString &path);

    /**
     * @brief 清理过期缓存
     * @param maxAgeHours 最大缓存时间（小时）
     */
    void cleanupExpiredCache(int maxAgeHours = 168); // 默认 7 天

    /**
     * @brief 清空所有缓存
     */
    void clearCache();

    /**
     * @brief 保存缓存到磁盘
     */
    void saveCache();

    /**
     * @brief 从磁盘加载缓存
     */
    void loadCache();

    /**
     * @brief 启动后台验证（异步更新过期缓存）
     */
    void startBackgroundValidation();

    /**
     * @brief 停止后台验证（等待当前验证完成）
     */
    void stopBackgroundValidation();

    /**
     * @brief 检查后台验证是否正在运行
     * @return 是否正在运行
     */
    bool isBackgroundValidationRunning() const { return m_backgroundValidationRunning; }

    /**
     * @brief 获取缓存统计信息
     * @return 统计信息
     */
    EnvironmentCacheStats getStats() const;

    /**
     * @brief 设置最后使用的环境
     * @param path Python 可执行文件路径
     */
    void setLastUsedEnvironment(const QString &path);

    /**
     * @brief 获取最后使用的环境
     * @return 环境路径
     */
    QString getLastUsedEnvironment() const;

    /**
     * @brief 设置最后使用的模型
     * @param path 模型文件路径
     */
    void setLastUsedModel(const QString &path);

    /**
     * @brief 获取最后使用的模型
     * @return 模型路径
     */
    QString getLastUsedModel() const;

    /**
     * @brief 获取缓存文件路径
     * @return 缓存文件路径
     */
    static QString getCacheFilePath();

    /**
     * @brief 获取状态文件路径
     * @return 状态文件路径
     */
    static QString getStateFilePath();

signals:
    /**
     * @brief 环境缓存更新信号
     */
    void environmentCacheUpdated(const QVector<CachedEnvironment> &envs);

    /**
     * @brief 单个环境验证完成信号
     */
    void environmentValidated(const CachedEnvironment &env);

    /**
     * @brief 后台验证进度信号
     */
    void backgroundValidationProgress(int current, int total);

    /**
     * @brief 后台验证完成信号
     */
    void backgroundValidationCompleted();

private:
    explicit EnvironmentCacheManager(QObject *parent = nullptr);
    ~EnvironmentCacheManager();

    // 禁止拷贝
    EnvironmentCacheManager(const EnvironmentCacheManager&) = delete;
    EnvironmentCacheManager& operator=(const EnvironmentCacheManager&) = delete;

    /**
     * @brief 验证单个环境（内部使用）
     */
    CachedEnvironment validateEnvironment(const QString &path);

    /**
     * @brief 后台验证工作函数
     */
    void backgroundValidationWorker();

    mutable QReadWriteLock m_cacheLock;
    mutable QMutex m_stateMutex;

    QVector<CachedEnvironment> m_cache;
    QString m_lastUsedEnvironment;
    QString m_lastUsedModel;
    bool m_backgroundValidationRunning = false;
    bool m_shouldStopBackgroundValidation = false;

    static EnvironmentCacheManager *s_instance;
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // ENVIRONMENTCACHEMANAGER_H
