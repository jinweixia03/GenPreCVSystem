/**
 * @file environmentcachemanager.cpp
 * @brief Python 环境缓存管理器实现
 *
 * 实现高性能的环境状态缓存：
 * - 快速启动时无需重新验证环境
 * - 后台异步验证更新缓存
 * - 持久化缓存到磁盘
 */

#include "environmentcachemanager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QProcess>
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <QDebug>

namespace GenPreCVSystem {
namespace Utils {

// 单例实例
EnvironmentCacheManager *EnvironmentCacheManager::s_instance = nullptr;

// 缓存版本号（用于缓存格式升级时使旧缓存失效）
static const int CACHE_VERSION = 1;

// 验证超时时间（毫秒）
static const int VALIDATION_TIMEOUT_MS = 5000;

CachedEnvironment::CachedEnvironment(const PythonEnvironment &env)
    : PythonEnvironment(env)
    , isValid(false)
    , hasTorch(false)
    , hasOpenCV(false)
    , hasNumPy(false)
    , validationTimeMs(0)
    , cudaAvailable(false)
    , cudaDeviceCount(0)
{
}

QString CachedEnvironment::getGpuStatusString() const
{
    if (!hasTorch) {
        return "No PyTorch";
    }
    if (!cudaAvailable || cudaDeviceCount == 0) {
        return "CPU Only";
    }
    QString status = gpuName;
    if (!gpuMemory.isEmpty()) {
        status += QString(" (%1)").arg(gpuMemory);
    }
    if (cudaDeviceCount > 1) {
        status += QString(" [x%1]").arg(cudaDeviceCount);
    }
    return status;
}

bool CachedEnvironment::isExpired(int maxAgeHours) const
{
    if (cacheUpdatedAt.isNull()) {
        return true;
    }
    return cacheUpdatedAt.secsTo(QDateTime::currentDateTime()) > (maxAgeHours * 3600);
}

bool CachedEnvironment::needsRevalidation() const
{
    // 如果没有验证过，需要验证
    if (validatedAt.isNull()) {
        return true;
    }

    // 如果超过 24 小时没有验证，需要重新验证
    if (validatedAt.secsTo(QDateTime::currentDateTime()) > (24 * 3600)) {
        return true;
    }

    // 如果环境无效但之前是有效的，需要验证（可能已修复）
    if (!isValid && cacheUpdatedAt.secsTo(QDateTime::currentDateTime()) > 3600) {
        return true;
    }

    return false;
}

EnvironmentCacheManager::EnvironmentCacheManager(QObject *parent)
    : QObject(parent)
    , m_backgroundValidationRunning(false)
    , m_shouldStopBackgroundValidation(false)
{
}

EnvironmentCacheManager::~EnvironmentCacheManager()
{
    // 停止后台验证，避免析构时线程还在运行
    if (m_backgroundValidationRunning) {
        stopBackgroundValidation();
    }
    saveCache();
}

EnvironmentCacheManager* EnvironmentCacheManager::instance()
{
    if (!s_instance) {
        s_instance = new EnvironmentCacheManager();
    }
    return s_instance;
}

void EnvironmentCacheManager::initialize(bool enableBackgroundValidation)
{
    loadCache();

    // 启动时清理过期缓存（超过 7 天）
    cleanupExpiredCache(168);

    // 启动后台验证（异步更新过期的缓存项）
    if (enableBackgroundValidation) {
        startBackgroundValidation();
    }
}

QVector<CachedEnvironment> EnvironmentCacheManager::getCachedEnvironments() const
{
    QReadLocker locker(&m_cacheLock);
    return m_cache;
}

QVector<CachedEnvironment> EnvironmentCacheManager::getReadyEnvironments() const
{
    QReadLocker locker(&m_cacheLock);
    QVector<CachedEnvironment> ready;
    for (const auto &env : m_cache) {
        if (env.isValid && env.hasUltralytics) {
            ready.append(env);
        }
    }
    return ready;
}

CachedEnvironment EnvironmentCacheManager::getEnvironment(const QString &path) const
{
    QReadLocker locker(&m_cacheLock);
    QString normalizedPath = QDir::cleanPath(path);
    for (const auto &env : m_cache) {
        if (QDir::cleanPath(env.path) == normalizedPath) {
            return env;
        }
    }
    return CachedEnvironment();
}

bool EnvironmentCacheManager::isEnvironmentValid(const QString &path) const
{
    CachedEnvironment env = getEnvironment(path);
    return env.isValid && QFile::exists(env.path);
}

bool EnvironmentCacheManager::isEnvironmentReady(const QString &path) const
{
    CachedEnvironment env = getEnvironment(path);
    return env.isValid && env.hasUltralytics && QFile::exists(env.path);
}

CachedEnvironment EnvironmentCacheManager::quickValidate(const QString &path)
{
    // 首先检查缓存
    CachedEnvironment cached = getEnvironment(path);

    // 如果缓存有效且未过期，直接返回
    if (cached.isValid && !cached.needsRevalidation()) {
        return cached;
    }

    // 如果路径不存在，返回无效
    if (!QFile::exists(path)) {
        cached.isValid = false;
        return cached;
    }

    // 缓存过期或不存在，执行完整验证
    return fullValidate(path);
}

CachedEnvironment EnvironmentCacheManager::fullValidate(const QString &path)
{
    CachedEnvironment result = validateEnvironment(path);
    updateCache(result);
    return result;
}

CachedEnvironment EnvironmentCacheManager::validateEnvironment(const QString &path)
{
    QElapsedTimer timer;
    timer.start();

    CachedEnvironment env;
    env.path = QDir::cleanPath(path);
    env.name = QFileInfo(path).dir().dirName();
    env.isValid = QFile::exists(path);

    if (!env.isValid) {
        env.validationTimeMs = timer.elapsed();
        return env;
    }

    // 检测环境类型
    QString dirPath = QFileInfo(path).dir().absolutePath();
    if (dirPath.contains("conda") || dirPath.contains("anaconda") || dirPath.contains("miniconda")) {
        env.type = "conda";
    } else if (dirPath.contains("venv") || dirPath.contains("virtualenv") || dirPath.contains("env")) {
        env.type = "venv";
    } else {
        env.type = "system";
    }

    // 执行验证脚本（包含 GPU 检测）
    QString checkScript = R"(
import sys
import json

result = {
    "python_version": ".".join(map(str, sys.version_info[:3])),
    "has_ultralytics": False,
    "ultralytics_version": None,
    "has_torch": False,
    "torch_version": None,
    "has_opencv": False,
    "has_numpy": False,
    "cuda_available": False,
    "cuda_device_count": 0,
    "cuda_version": None,
    "gpu_name": None,
    "gpu_memory": None
}

try:
    import ultralytics
    result["has_ultralytics"] = True
    result["ultralytics_version"] = ultralytics.__version__
except ImportError:
    pass

try:
    import torch
    result["has_torch"] = True
    result["torch_version"] = torch.__version__

    # GPU 检测
    result["cuda_available"] = torch.cuda.is_available()
    if result["cuda_available"]:
        result["cuda_device_count"] = torch.cuda.device_count()
        if result["cuda_device_count"] > 0:
            # 获取第一个 GPU 的信息
            device_props = torch.cuda.get_device_properties(0)
            result["gpu_name"] = device_props.name
            # 显存信息（MB）
            total_memory_mb = device_props.total_memory // (1024 * 1024)
            result["gpu_memory"] = f"{total_memory_mb}MB"
            # CUDA 版本
            result["cuda_version"] = torch.version.cuda
except ImportError:
    pass

try:
    import cv2
    result["has_opencv"] = True
except ImportError:
    pass

try:
    import numpy
    result["has_numpy"] = True
except ImportError:
    pass

print(json.dumps(result))
)";

    QProcess process;
    process.setProgram(path);
    process.setArguments(QStringList() << "-c" << checkScript);
    process.start();

    // 首先等待进程启动，如果启动失败直接返回
    if (!process.waitForStarted(1000)) {
        env.validationTimeMs = timer.elapsed();
        return env;
    }

    // 等待进程完成
    if (process.waitForFinished(VALIDATION_TIMEOUT_MS)) {
        if (process.exitCode() == 0) {
            QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());

            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                env.pythonVersion = obj["python_version"].toString();
                env.hasUltralytics = obj["has_ultralytics"].toBool();
                env.ultralyticsVersion = obj["ultralytics_version"].toString();
                env.hasTorch = obj["has_torch"].toBool();
                env.torchVersion = obj["torch_version"].toString();
                env.hasOpenCV = obj["has_opencv"].toBool();
                env.hasNumPy = obj["has_numpy"].toBool();

                // GPU 信息
                env.cudaAvailable = obj["cuda_available"].toBool();
                env.cudaDeviceCount = obj["cuda_device_count"].toInt();
                env.cudaVersion = obj["cuda_version"].toString();
                env.gpuName = obj["gpu_name"].toString();
                env.gpuMemory = obj["gpu_memory"].toString();
            }
        }
    }

    env.validatedAt = QDateTime::currentDateTime();
    env.cacheUpdatedAt = env.validatedAt;
    env.validationTimeMs = timer.elapsed();

    return env;
}

void EnvironmentCacheManager::updateCache(const CachedEnvironment &env)
{
    QWriteLocker locker(&m_cacheLock);

    QString normalizedPath = QDir::cleanPath(env.path);
    bool found = false;
    for (int i = 0; i < m_cache.size(); ++i) {
        if (QDir::cleanPath(m_cache[i].path) == normalizedPath) {
            m_cache[i] = env;
            found = true;
            break;
        }
    }

    if (!found) {
        m_cache.append(env);
    }

    locker.unlock();

    // 异步保存缓存
    (void)QtConcurrent::run([this]() {
        saveCache();
    });

    emit environmentValidated(env);
}

void EnvironmentCacheManager::updateCacheBatch(const QVector<CachedEnvironment> &envs)
{
    QWriteLocker locker(&m_cacheLock);

    for (const auto &env : envs) {
        QString normalizedPath = QDir::cleanPath(env.path);
        bool found = false;
        for (int i = 0; i < m_cache.size(); ++i) {
            if (QDir::cleanPath(m_cache[i].path) == normalizedPath) {
                m_cache[i] = env;
                found = true;
                break;
            }
        }
        if (!found) {
            m_cache.append(env);
        }
    }

    locker.unlock();

    // 异步保存缓存
    (void)QtConcurrent::run([this]() {
        saveCache();
    });

    emit environmentCacheUpdated(m_cache);
}

void EnvironmentCacheManager::removeFromCache(const QString &path)
{
    QWriteLocker locker(&m_cacheLock);
    QString normalizedPath = QDir::cleanPath(path);
    for (int i = 0; i < m_cache.size(); ++i) {
        if (QDir::cleanPath(m_cache[i].path) == normalizedPath) {
            m_cache.removeAt(i);
            break;
        }
    }
}

void EnvironmentCacheManager::cleanupExpiredCache(int maxAgeHours)
{
    QWriteLocker locker(&m_cacheLock);

    QDateTime now = QDateTime::currentDateTime();
    for (int i = m_cache.size() - 1; i >= 0; --i) {
        // 如果缓存超过 maxAgeHours 且路径不存在，删除
        if (m_cache[i].cacheUpdatedAt.secsTo(now) > (maxAgeHours * 3600)) {
            if (!QFile::exists(m_cache[i].path)) {
                m_cache.removeAt(i);
            }
        }
    }
}

void EnvironmentCacheManager::clearCache()
{
    // 设置停止标志，但不等待（避免卡在 QProcess）
    m_shouldStopBackgroundValidation = true;

    QWriteLocker locker(&m_cacheLock);
    m_cache.clear();
    locker.unlock();

    saveCache();
}

QString EnvironmentCacheManager::getCacheFilePath()
{
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return cacheDir + "/environment_cache_v" + QString::number(CACHE_VERSION) + ".json";
}

QString EnvironmentCacheManager::getStateFilePath()
{
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return cacheDir + "/environment_state.json";
}

void EnvironmentCacheManager::saveCache()
{
    // 保存环境缓存
    QJsonArray envArray;
    {
        QReadLocker locker(&m_cacheLock);
        for (const auto &env : m_cache) {
            QJsonObject obj;
            obj["name"] = env.name;
            obj["path"] = env.path;
            obj["type"] = env.type;
            obj["isValid"] = env.isValid;
            obj["hasUltralytics"] = env.hasUltralytics;
            obj["hasTorch"] = env.hasTorch;
            obj["hasOpenCV"] = env.hasOpenCV;
            obj["hasNumPy"] = env.hasNumPy;
            obj["pythonVersion"] = env.pythonVersion;
            obj["ultralyticsVersion"] = env.ultralyticsVersion;
            obj["torchVersion"] = env.torchVersion;
            obj["validatedAt"] = env.validatedAt.toString(Qt::ISODate);
            obj["cacheUpdatedAt"] = env.cacheUpdatedAt.toString(Qt::ISODate);
            obj["validationTimeMs"] = env.validationTimeMs;
            // GPU 信息
            obj["cudaAvailable"] = env.cudaAvailable;
            obj["cudaDeviceCount"] = env.cudaDeviceCount;
            obj["cudaVersion"] = env.cudaVersion;
            obj["gpuName"] = env.gpuName;
            obj["gpuMemory"] = env.gpuMemory;
            envArray.append(obj);
        }
    }

    QJsonObject cacheDoc;
    cacheDoc["version"] = CACHE_VERSION;
    cacheDoc["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    cacheDoc["environments"] = envArray;

    QFile cacheFile(getCacheFilePath());
    if (cacheFile.open(QIODevice::WriteOnly)) {
        cacheFile.write(QJsonDocument(cacheDoc).toJson(QJsonDocument::Compact));
    }

    // 保存状态（最后使用的环境和模型）
    QJsonObject stateObj;
    {
        QMutexLocker locker(&m_stateMutex);
        stateObj["lastUsedEnvironment"] = m_lastUsedEnvironment;
        stateObj["lastUsedModel"] = m_lastUsedModel;
    }
    stateObj["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile stateFile(getStateFilePath());
    if (stateFile.open(QIODevice::WriteOnly)) {
        stateFile.write(QJsonDocument(stateObj).toJson(QJsonDocument::Compact));
    }
}

void EnvironmentCacheManager::loadCache()
{
    // 加载环境缓存
    QFile cacheFile(getCacheFilePath());
    if (cacheFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(cacheFile.readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject root = doc.object();
            int version = root["version"].toInt();

            // 版本检查
            if (version == CACHE_VERSION) {
                QJsonArray envArray = root["environments"].toArray();
                QWriteLocker locker(&m_cacheLock);
                m_cache.clear();

                for (const auto &val : envArray) {
                    QJsonObject obj = val.toObject();
                    CachedEnvironment env;
                    env.name = obj["name"].toString();
                    env.path = obj["path"].toString();
                    env.type = obj["type"].toString();
                    env.isValid = obj["isValid"].toBool();
                    env.hasUltralytics = obj["hasUltralytics"].toBool();
                    env.hasTorch = obj["hasTorch"].toBool();
                    env.hasOpenCV = obj["hasOpenCV"].toBool();
                    env.hasNumPy = obj["hasNumPy"].toBool();
                    env.pythonVersion = obj["pythonVersion"].toString();
                    env.ultralyticsVersion = obj["ultralyticsVersion"].toString();
                    env.torchVersion = obj["torchVersion"].toString();
                    env.validatedAt = QDateTime::fromString(obj["validatedAt"].toString(), Qt::ISODate);
                    env.cacheUpdatedAt = QDateTime::fromString(obj["cacheUpdatedAt"].toString(), Qt::ISODate);
                    env.validationTimeMs = obj["validationTimeMs"].toInt();
                    // GPU 信息
                    env.cudaAvailable = obj["cudaAvailable"].toBool();
                    env.cudaDeviceCount = obj["cudaDeviceCount"].toInt();
                    env.cudaVersion = obj["cudaVersion"].toString();
                    env.gpuName = obj["gpuName"].toString();
                    env.gpuMemory = obj["gpuMemory"].toString();

                    // 验证路径是否仍然存在
                    env.isValid = env.isValid && QFile::exists(env.path);

                    m_cache.append(env);
                }
            }
        }
    }

    // 加载状态
    QFile stateFile(getStateFilePath());
    if (stateFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(stateFile.readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            QMutexLocker locker(&m_stateMutex);
            m_lastUsedEnvironment = obj["lastUsedEnvironment"].toString();
            m_lastUsedModel = obj["lastUsedModel"].toString();
        }
    }
}

void EnvironmentCacheManager::startBackgroundValidation()
{
    if (m_backgroundValidationRunning) {
        return;
    }

    m_shouldStopBackgroundValidation = false;
    m_backgroundValidationRunning = true;

    (void)QtConcurrent::run([this]() {
        backgroundValidationWorker();
    });
}

void EnvironmentCacheManager::stopBackgroundValidation()
{
    m_shouldStopBackgroundValidation = true;
    // 等待后台验证完成（最多等待 10 秒）
    int waitCount = 0;
    while (m_backgroundValidationRunning && waitCount < 100) {
        QThread::msleep(100);
        waitCount++;
    }
}

void EnvironmentCacheManager::backgroundValidationWorker()
{
    QVector<CachedEnvironment> toValidate;

    {
        QReadLocker locker(&m_cacheLock);
        for (const auto &env : m_cache) {
            if (m_shouldStopBackgroundValidation) break;
            if (env.needsRevalidation()) {
                toValidate.append(env);
            }
        }
    }

    int total = toValidate.size();
    int current = 0;

    for (const auto &env : toValidate) {
        // 检查是否应该停止
        if (m_shouldStopBackgroundValidation) {
            break;
        }

        // 检查路径是否仍然存在
        if (!QFile::exists(env.path)) {
            removeFromCache(env.path);
            continue;
        }

        // 验证环境
        CachedEnvironment validated = validateEnvironment(env.path);
        updateCache(validated);

        current++;
        emit backgroundValidationProgress(current, total);

        // 短暂休眠，避免 CPU 占用过高
        QThread::msleep(100);
    }

    m_backgroundValidationRunning = false;
    m_shouldStopBackgroundValidation = false;
    emit backgroundValidationCompleted();
}

EnvironmentCacheStats EnvironmentCacheManager::getStats() const
{
    EnvironmentCacheStats stats;
    QReadLocker locker(&m_cacheLock);

    stats.totalEnvironments = m_cache.size();

    int totalValidationTime = 0;
    for (const auto &env : m_cache) {
        if (env.isValid) {
            stats.validEnvironments++;
        }
        if (env.isValid && env.hasUltralytics) {
            stats.readyEnvironments++;
        }
        if (env.isExpired()) {
            stats.expiredEnvironments++;
        }
        totalValidationTime += env.validationTimeMs;
    }

    if (!m_cache.isEmpty()) {
        stats.averageValidationTimeMs = totalValidationTime / m_cache.size();
    }

    return stats;
}

void EnvironmentCacheManager::setLastUsedEnvironment(const QString &path)
{
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastUsedEnvironment = path;
    }
    // 异步保存
    (void)QtConcurrent::run([this]() {
        saveCache();
    });
}

QString EnvironmentCacheManager::getLastUsedEnvironment() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_lastUsedEnvironment;
}

void EnvironmentCacheManager::setLastUsedModel(const QString &path)
{
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastUsedModel = path;
    }
    // 异步保存
    (void)QtConcurrent::run([this]() {
        saveCache();
    });
}

QString EnvironmentCacheManager::getLastUsedModel() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_lastUsedModel;
}

} // namespace Utils
} // namespace GenPreCVSystem
