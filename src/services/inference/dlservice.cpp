/**
 * @file dlservice.cpp
 * @brief 深度学习推理服务实现
 *
 * 实现深度学习模型的 Python 后端通信，支持：
 * - 目标检测 (Object Detection)
 * - 实例分割 (Instance Segmentation)
 * - 图像分类 (Image Classification)
 * - 姿态/关键点检测 (Pose/Keypoint Detection)
 *
 * 通过 QProcess 与 Python 脚本通信，使用 JSON 格式传输数据
 */

#include "dlservice.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QElapsedTimer>
#include <QSettings>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QTextStream>
#include <QSet>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

namespace GenPreCVSystem {
namespace Utils {

// 默认的 conda 环境名称
static const QString DEFAULT_CONDA_ENV = "GenPreCVSystem";

// 缓存文件路径（向后兼容）
static QString getCacheFilePath() {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return cacheDir + "/python_env_cache.json";
}

// 保存环境缓存（向后兼容）
static void saveEnvironmentCache(const QVector<PythonEnvironment> &envs) {
    QJsonArray array;
    for (const auto &env : envs) {
        QJsonObject obj;
        obj["name"] = env.name;
        obj["path"] = env.path;
        obj["type"] = env.type;
        obj["hasUltralytics"] = env.hasUltralytics;
        array.append(obj);
    }
    QJsonDocument doc(array);
    QFile file(getCacheFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

// 加载环境缓存（向后兼容）
static QVector<PythonEnvironment> loadEnvironmentCache() {
    QVector<PythonEnvironment> result;
    QFile file(getCacheFilePath());
    if (!file.exists()) return result;

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray array = doc.array();
        for (const auto &val : array) {
            QJsonObject obj = val.toObject();
            PythonEnvironment env;
            env.name = obj["name"].toString();
            env.path = obj["path"].toString();
            env.type = obj["type"].toString();
            env.hasUltralytics = obj["hasUltralytics"].toBool();
            // 验证路径是否仍然有效
            if (QFile::exists(env.path)) {
                result.append(env);
            }
        }
    }
    return result;
}

DLService::DLService(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_modelLoaded(false)
    , m_taskType("dl")  // 默认使用 DL 服务
{
}

DLService::~DLService()
{
    stop();
}

QVector<PythonEnvironment> DLService::scanEnvironments()
{
    QVector<PythonEnvironment> environments;
    QString homeDir = QDir::homePath();
    QSet<QString> addedPaths;  // 用于去重

    // 辅助函数：添加环境到列表（快速模式，不检查 ultralytics）
    auto addEnvironment = [&](const QString &name, const QString &pythonPath, const QString &type, bool checkUltralytics = false) {
        QString normalizedPath = QDir::cleanPath(pythonPath);
        if (addedPaths.contains(normalizedPath) || !QFile::exists(normalizedPath)) {
            return;
        }
        addedPaths.insert(normalizedPath);

        PythonEnvironment env;
        env.name = name;
        env.path = normalizedPath;
        env.type = type;

        // 可选：快速检查是否有 ultralytics（超时更短）
        if (checkUltralytics) {
            QProcess checkUltra;
            checkUltra.start(normalizedPath, QStringList() << "-c" << "import ultralytics; print('ok')");
            // 缩短超时时间到 2 秒
            if (checkUltra.waitForFinished(2000) && checkUltra.exitCode() == 0) {
                env.hasUltralytics = true;
            } else {
                env.hasUltralytics = false;
            }
        } else {
            // 默认假设没有 ultralytics，延迟到实际使用时检查
            env.hasUltralytics = false;
        }
        environments.append(env);
    };

    // 1. 添加系统 Python（快速检查，超时 1 秒）
    QProcess sysPython;
    sysPython.setProgram("python");
    sysPython.setArguments(QStringList() << "--version");
    sysPython.start();
    // 首先等待进程启动，如果启动失败直接跳过
    if (sysPython.waitForStarted(500)) {
        // 进程启动成功，等待完成
        if (sysPython.waitForFinished(1000)) {  // 缩短超时
            if (sysPython.exitCode() == 0) {
                addEnvironment("系统 Python", "python", "system", false);
            }
        }
    } else {
        qDebug() << "[DLService] python 命令不可用，跳过系统 Python 扫描";
    }

    // 2. 读取 ~/.conda/environments.txt 获取所有 conda 环境路径（快速方式）
    QString envFilePath = homeDir + "/.conda/environments.txt";
    QFile envFile(envFilePath);
    if (envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&envFile);
        while (!in.atEnd()) {
            QString envPath = in.readLine().trimmed();
            if (envPath.isEmpty()) continue;

            QString normalizedPath = QDir::cleanPath(envPath);
            QString envName = QFileInfo(normalizedPath).fileName();
            if (envName.isEmpty()) continue;

            // 判断是否是 base 环境
            QDir dir(normalizedPath);
            QString parentName = dir.cdUp() ? dir.dirName() : QString();
            if (parentName.compare("envs", Qt::CaseInsensitive) != 0) {
                envName = "base";
            }

            // 构建 Python 路径
            QString pythonPath;
#ifdef Q_OS_WIN
            pythonPath = normalizedPath + "/python.exe";
#else
            pythonPath = normalizedPath + "/bin/python";
#endif

            if (QFile::exists(pythonPath)) {
                addEnvironment(envName, pythonPath, "conda", false);
            }
        }
        envFile.close();
    }

    // 3. 使用 conda env list 获取所有 conda 环境（带超时保护）
    QProcess condaProcess;
    condaProcess.setProgram("conda");
    condaProcess.setArguments(QStringList() << "env" << "list");
    condaProcess.start();

    // 检查进程是否成功启动（更短的超时）
    if (!condaProcess.waitForStarted(500)) {
        qDebug() << "[DLService] conda 命令不可用，跳过 conda env list 扫描";
        // 确保进程被终止
        if (condaProcess.state() != QProcess::NotRunning) {
            condaProcess.kill();
            condaProcess.waitForFinished(100);
        }
    } else {
        // 缩短超时到 3 秒，避免卡住
        if (condaProcess.waitForFinished(3000)) {
            if (condaProcess.exitCode() == 0) {
                QString output = QString::fromUtf8(condaProcess.readAllStandardOutput());
                QStringList lines = output.split('\n');

                for (const QString &line : lines) {
                    QString trimmed = line.trimmed();
                    if (trimmed.isEmpty() || trimmed.startsWith('#')) {
                        continue;
                    }

                    // 格式: name path 或 name * path
                    QRegularExpression regex(R"(^(\S+)\s+(\*?\s*)?(\S+)$)");
                    QRegularExpressionMatch match = regex.match(trimmed);

                    if (match.hasMatch()) {
                        QString envName = match.captured(1);
                        QString envPath = match.captured(3);

                        QString pythonPath;
#ifdef Q_OS_WIN
                        pythonPath = envPath + "/python.exe";
#else
                        pythonPath = envPath + "/bin/python";
#endif

                        if (QFile::exists(pythonPath)) {
                            addEnvironment(envName, pythonPath, "conda", false);
                        }
                    }
                }
            } else {
                QString errorOutput = QString::fromUtf8(condaProcess.readAllStandardError());
                qDebug() << "[DLService] conda env list 执行失败:" << errorOutput;
            }
        } else {
            qDebug() << "[DLService] conda env list 执行超时";
            condaProcess.kill();
        }
    }
    // 注意：如果 conda 命令超时或失败，我们已经通过 environments.txt 扫描了大部分环境

    // 4. 扫描常见的 conda 安装目录（作为备用，限制扫描数量避免过慢）
    QStringList condaEnvDirs = {
        homeDir + "/miniconda3/envs",
        homeDir + "/anaconda3/envs",
        homeDir + "/Miniconda3/envs",
        homeDir + "/Anaconda3/envs",
        homeDir + "/.conda/envs",
        "C:/ProgramData/miniconda3/envs",
        "C:/ProgramData/anaconda3/envs",
    };

    int scannedCount = 0;
    const int MAX_SCAN = 20;  // 限制最大扫描数量

    for (const QString &envDir : condaEnvDirs) {
        if (scannedCount >= MAX_SCAN) break;

        QDir dir(envDir);
        if (!dir.exists()) continue;

        QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subDir : subDirs) {
            if (scannedCount >= MAX_SCAN) break;

            QString pythonPath;
#ifdef Q_OS_WIN
            pythonPath = envDir + "/" + subDir + "/python.exe";
#else
            pythonPath = envDir + "/" + subDir + "/bin/python";
#endif
            addEnvironment(subDir, pythonPath, "conda", false);
            scannedCount++;
        }
    }

    // 5. 检测 conda base 环境
    QStringList condaBasePaths = {
        homeDir + "/miniconda3",
        homeDir + "/anaconda3",
        homeDir + "/Miniconda3",
        homeDir + "/Anaconda3",
        "C:/ProgramData/miniconda3",
        "C:/ProgramData/anaconda3",
        "C:/miniconda3",
        "C:/anaconda3",
    };

    for (const QString &basePath : condaBasePaths) {
        QString pythonPath;
#ifdef Q_OS_WIN
        pythonPath = basePath + "/python.exe";
#else
        pythonPath = basePath + "/bin/python";
#endif
        if (QFile::exists(pythonPath)) {
            addEnvironment("base", pythonPath, "conda", false);
        }
    }

    // 6. 扫描 venv 虚拟环境（常见位置）
    QStringList venvSearchDirs = {
        homeDir + "/.venvs",
        homeDir + "/venvs",
        homeDir + "/virtualenvs",
        homeDir + "/Envs",
    };

    // 添加当前工作目录下的常见 venv 目录
    QString currentDir = QDir::currentPath();
    venvSearchDirs.append(currentDir + "/venv");
    venvSearchDirs.append(currentDir + "/.venv");
    venvSearchDirs.append(currentDir + "/env");

    for (const QString &venvDir : venvSearchDirs) {
        QString pythonPath;
#ifdef Q_OS_WIN
        pythonPath = venvDir + "/Scripts/python.exe";
#else
        pythonPath = venvDir + "/bin/python";
#endif
        if (QFile::exists(pythonPath)) {
            QString venvName = QFileInfo(venvDir).fileName();
            addEnvironment(venvName, pythonPath, "venv", false);
        }
    }

    // 7. 扫描 Windows 常见的 Python 安装路径
#ifdef Q_OS_WIN
    QStringList winPythonPaths = {
        "C:/Python311/python.exe",
        "C:/Python310/python.exe",
        "C:/Python39/python.exe",
        "C:/Program Files/Python311/python.exe",
        "C:/Program Files/Python310/python.exe",
        "C:/Program Files/Python39/python.exe",
        "C:/Program Files (x86)/Python311-32/python.exe",
        "C:/Program Files (x86)/Python310-32/python.exe",
    };

    for (const QString &pyPath : winPythonPaths) {
        if (QFile::exists(pyPath)) {
            addEnvironment("系统 Python", pyPath, "system", false);
        }
    }
#endif

    // 转换为 CachedEnvironment 并保存到缓存管理器
    QVector<CachedEnvironment> cachedEnvs;
    for (const auto &env : environments) {
        cachedEnvs.append(CachedEnvironment(env));
    }
    EnvironmentCacheManager::instance()->updateCacheBatch(cachedEnvs);

    // 同时保存到旧缓存（向后兼容）
    saveEnvironmentCache(environments);

    return environments;
}

// 快速获取环境列表（优先使用缓存）
QVector<PythonEnvironment> DLService::getEnvironments()
{
    // 1. 首先尝试从缓存管理器获取
    EnvironmentCacheManager *cacheMgr = EnvironmentCacheManager::instance();
    QVector<CachedEnvironment> cached = cacheMgr->getCachedEnvironments();

    if (!cached.isEmpty()) {
        // 转换为普通 PythonEnvironment
        QVector<PythonEnvironment> result;
        for (const auto &env : cached) {
            PythonEnvironment basic;
            basic.name = env.name;
            basic.path = env.path;
            basic.type = env.type;
            basic.hasUltralytics = env.hasUltralytics;
            result.append(basic);
        }

        // 触发后台刷新（异步更新缓存）
        refreshEnvironmentCacheAsync();
        cacheMgr->startBackgroundValidation();

        return result;
    }

    // 2. 尝试加载旧缓存格式
    QVector<PythonEnvironment> oldCached = loadEnvironmentCache();
    if (!oldCached.isEmpty()) {
        // 迁移到新的缓存管理器
        QVector<CachedEnvironment> toMigrate;
        for (const auto &env : oldCached) {
            toMigrate.append(CachedEnvironment(env));
        }
        cacheMgr->updateCacheBatch(toMigrate);

        refreshEnvironmentCacheAsync();
        return oldCached;
    }

    // 3. 缓存不存在，执行完整扫描
    return scanEnvironments();
}

// 验证指定环境是否安装了 ultralytics
bool DLService::checkEnvironmentCapability(PythonEnvironment &env)
{
    // 使用缓存管理器快速验证
    CachedEnvironment cached = EnvironmentCacheManager::instance()->quickValidate(env.path);
    env.hasUltralytics = cached.hasUltralytics;
    return cached.hasUltralytics;
}

// 快速启动服务（使用缓存验证）
bool DLService::fastStart()
{
    if (m_process && m_process->state() == QProcess::Running) {
        emit logMessage("服务已在运行中");
        return true;
    }

    // 确定要使用的环境
    QString envPath = m_environmentPath;
    if (envPath.isEmpty()) {
        envPath = EnvironmentCacheManager::instance()->getLastUsedEnvironment();
    }

    // 检查是否可以使用快速启动
    if (!canFastStart(envPath)) {
        emit logMessage("环境未验证，执行完整启动...");
        return start();  // 回退到正常启动
    }

    // 使用缓存的环境信息直接启动
    m_environmentPath = envPath;
    return start(m_environmentPath);
}

// 检查指定环境是否可以直接启动（使用缓存）
bool DLService::canFastStart(const QString &envPath) const
{
    if (envPath.isEmpty()) {
        return false;
    }

    // 检查缓存中该环境是否已就绪
    return EnvironmentCacheManager::instance()->isEnvironmentReady(envPath);
}

// 获取上次使用的环境路径（从缓存）
QString DLService::getLastUsedEnvironment() const
{
    return EnvironmentCacheManager::instance()->getLastUsedEnvironment();
}

// 获取上次使用的模型路径（从缓存）
QString DLService::getLastUsedModel() const
{
    return EnvironmentCacheManager::instance()->getLastUsedModel();
}

// 异步刷新环境缓存（后台线程执行）
void DLService::refreshEnvironmentCacheAsync()
{
    // 使用 QtConcurrent 在后台线程执行扫描
    (void)QtConcurrent::run([]() {
        QVector<PythonEnvironment> envs = scanEnvironments();
        saveEnvironmentCache(envs);
    });
}

QString DLService::currentEnvironmentPath() const
{
    return m_environmentPath;
}

void DLService::setEnvironmentPath(const QString &path)
{
    m_environmentPath = path;

    // 保存到缓存管理器
    EnvironmentCacheManager::instance()->setLastUsedEnvironment(path);

    // 保存到配置文件（向后兼容）
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configPath);
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }

    QSettings settings(configPath + "/dl_settings.ini", QSettings::IniFormat);
    settings.setValue("python/path", path);
    settings.sync();
}

QString DLService::getDefaultScriptPath() const
{
    // 如果是 FSL 任务，返回 FSL 服务脚本
    if (m_taskType == "fsl" || m_taskType == "few_shot" || m_taskType == "RemoteSceneFewShotClassification") {
        return getFSLScriptPath();
    }

    // 查找默认脚本路径
    QString appDir = QCoreApplication::applicationDirPath();
    QDir appQDir(appDir);

    // 尝试多个可能的路径（按优先级排序）
    QStringList possiblePaths = {
        // 1. 构建目录中的 python 文件夹（CMake 复制后的位置，最高优先级）
        appDir + "/python/dl_service.py",
        // 2. 重构后的新位置（从源目录运行）
        appDir + "/../src/services/inference/python/dl_service.py",
        appDir + "/src/services/inference/python/dl_service.py",
        QCoreApplication::applicationDirPath() + "/../../src/services/inference/python/dl_service.py",
        // 3. 旧位置（向后兼容）
        appDir + "/../src/dl_service.py",
        appDir + "/src/dl_service.py",
        QCoreApplication::applicationDirPath() + "/../../src/dl_service.py",
        // 4. 其他可能的位置
        appDir + "/../src/dl/scripts/inference/dl_service.py",
        appDir + "/src/dl/scripts/inference/dl_service.py",
        QCoreApplication::applicationDirPath() + "/../../src/dl/scripts/inference/dl_service.py"
    };

    // 尝试从构建目录找到源目录
    QDir buildDir(appDir);
    if (buildDir.cdUp()) {
        // 构建目录的上级可能包含 src 目录
        possiblePaths.append(buildDir.absolutePath() + "/src/services/inference/python/dl_service.py");
        possiblePaths.append(buildDir.absolutePath() + "/src/dl_service.py");
        // 也可能有复制的 python 文件夹
        possiblePaths.append(buildDir.absolutePath() + "/python/dl_service.py");
    }

    for (const QString &path : possiblePaths) {
        QString normalized = QDir::cleanPath(path);
        if (QFile::exists(normalized)) {
            qDebug() << "[DLService] Found script at:" << normalized;
            return normalized;
        }
    }

    // 默认返回第一个路径（新位置）
    qDebug() << "[DLService] Script not found, returning default path:" << possiblePaths.first();
    return QDir::cleanPath(possiblePaths.first());
}

QString DLService::getFSLScriptPath() const
{
    // 查找 FSL 服务脚本路径
    QString appDir = QCoreApplication::applicationDirPath();
    QDir appQDir(appDir);

    // 尝试多个可能的路径（按优先级排序）
    QStringList possiblePaths = {
        // 1. 构建目录中的 python 文件夹（CMake 复制后的位置，最高优先级）
        appDir + "/python/fsl_service.py",
        // 2. 重构后的新位置（从源目录运行）
        appDir + "/../src/services/inference/python/fsl_service.py",
        appDir + "/src/services/inference/python/fsl_service.py",
        QCoreApplication::applicationDirPath() + "/../../src/services/inference/python/fsl_service.py",
        // 3. 当前目录
        appDir + "/fsl_service.py",
    };

    // 尝试从构建目录找到源目录
    QDir buildDir(appDir);
    if (buildDir.cdUp()) {
        possiblePaths.append(buildDir.absolutePath() + "/src/services/inference/python/fsl_service.py");
        possiblePaths.append(buildDir.absolutePath() + "/python/fsl_service.py");
    }

    for (const QString &path : possiblePaths) {
        QString normalized = QDir::cleanPath(path);
        if (QFile::exists(normalized)) {
            qDebug() << "[DLService] Found FSL script at:" << normalized;
            return normalized;
        }
    }

    // 默认返回第一个路径
    qDebug() << "[DLService] FSL Script not found, returning default path:" << possiblePaths.first();
    return QDir::cleanPath(possiblePaths.first());
}

QString DLService::findCondaPython() const
{
    // 1. 首先检查配置文件中是否有指定的 Python 路径
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configPath);
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }

    QSettings settings(configPath + "/dl_settings.ini", QSettings::IniFormat);
    QString savedPythonPath = settings.value("python/path").toString();
    if (!savedPythonPath.isEmpty() && QFile::exists(savedPythonPath)) {
        return savedPythonPath;
    }

    // 2. 尝试使用 conda run 命令（推荐方式）
    // 检查 conda 是否可用（超时 1.5 秒，避免卡住）
    QProcess testProcess;
    testProcess.start("conda", QStringList() << "--version");
    if (testProcess.waitForFinished(1500)) {
        if (testProcess.exitCode() == 0) {
            // conda 可用，使用 conda run 方式
            return "conda";  // 返回特殊标记，表示使用 conda run
        }
    }
    // 注意：如果 conda 命令超时或失败，继续尝试其他方式

    // 3. 尝试常见的 conda 安装路径 (Windows)
    QString homeDir = QDir::homePath();
    QStringList condaPaths = {
        // Miniconda/Anaconda 环境路径
        homeDir + "/miniconda3/envs/" + DEFAULT_CONDA_ENV + "/python.exe",
        homeDir + "/anaconda3/envs/" + DEFAULT_CONDA_ENV + "/python.exe",
        homeDir + "/Miniconda3/envs/" + DEFAULT_CONDA_ENV + "/python.exe",
        homeDir + "/Anaconda3/envs/" + DEFAULT_CONDA_ENV + "/python.exe",
        // 系统级安装
        "C:/ProgramData/miniconda3/envs/" + DEFAULT_CONDA_ENV + "/python.exe",
        "C:/ProgramData/anaconda3/envs/" + DEFAULT_CONDA_ENV + "/python.exe",
        // conda 环境中的 python
        homeDir + "/.conda/envs/" + DEFAULT_CONDA_ENV + "/python.exe",
    };

    for (const QString &path : condaPaths) {
        QString normalized = QDir::cleanPath(path);
        if (QFile::exists(normalized)) {
            return normalized;
        }
    }

    // 4. 回退到系统 python
    return "python";
}

bool DLService::start(const QString &pythonPath, const QString &scriptPath)
{
    if (m_process && m_process->state() == QProcess::Running) {
        emit logMessage("服务已在运行中");
        return true;
    }

    // 创建进程
    m_process = new QProcess(this);

    // 确定 Python 路径 (优先级: 参数 > m_environmentPath > 配置文件/自动检测)
    QString python;
    if (!pythonPath.isEmpty()) {
        python = pythonPath;
    } else if (!m_environmentPath.isEmpty() && QFile::exists(m_environmentPath)) {
        python = m_environmentPath;
    } else {
        python = findCondaPython();
    }

    // 确定脚本路径
    QString script = scriptPath.isEmpty() ? getDefaultScriptPath() : scriptPath;

    // 检查脚本文件是否存在
    emit logMessage(QString("[调试] 任务类型: %1").arg(m_taskType));
    emit logMessage(QString("[调试] Python 路径: %1").arg(python));
    emit logMessage(QString("[调试] 脚本路径: %1").arg(script));

    if (!QFile::exists(script)) {
        emit logMessage(QString("错误: 服务脚本不存在: %1").arg(script));
        delete m_process;
        m_process = nullptr;
        return false;
    }

    // 设置进程通道模式 - 分离 stderr 和 stdout
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    QStringList args;
    bool useCondaRun = false;

    if (python == "conda") {
        // 使用 conda run 方式运行
        useCondaRun = true;
        args << "run" << "-n" << DEFAULT_CONDA_ENV << "python" << script;
        emit logMessage(QString("启动服务: conda run -n %1 python %2").arg(DEFAULT_CONDA_ENV, script));
        m_process->start("conda", args);
    } else {
        // 直接使用 Python 路径
        args << script;
        emit logMessage(QString("启动服务: %1 %2").arg(python, script));
        m_process->start(python, args);
    }

    // 等待进程启动
    if (!m_process->waitForStarted(10000)) {
        QString errorMsg = m_process->errorString();
        emit logMessage("服务启动失败: " + errorMsg);

        // 如果使用 conda 失败，尝试直接使用 python
        if (useCondaRun) {
            emit logMessage("尝试使用系统 Python...");
            delete m_process;
            m_process = new QProcess(this);
            m_process->setProcessChannelMode(QProcess::SeparateChannels);
            QStringList fallbackArgs;
            fallbackArgs << script;
            m_process->start("python", fallbackArgs);

            if (!m_process->waitForStarted(5000)) {
                emit logMessage("系统 Python 也启动失败: " + m_process->errorString());
                delete m_process;
                m_process = nullptr;
                return false;
            }
        } else {
            delete m_process;
            m_process = nullptr;
            return false;
        }
    }

    // 等待就绪信号
    if (!m_process->waitForReadyRead(10000)) {
        emit logMessage("服务未响应 (可能 ultralytics 未安装)");
        stop();
        return false;
    }

    // 读取就绪响应
    QString response = QString::fromUtf8(m_process->readLine()).trimmed();
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    if (doc.isNull() || !doc.object()["success"].toBool()) {
        QString msg = doc.isNull() ? "无效响应" : doc.object()["message"].toString();
        emit logMessage("服务初始化失败: " + msg);
        stop();
        return false;
    }

    emit logMessage("服务启动成功");
    emit serviceStateChanged(true);
    return true;
}

void DLService::stop()
{
    if (m_process) {
        if (m_process->state() == QProcess::Running) {
            // 发送退出命令
            QJsonObject exitCmd;
            exitCmd["command"] = "exit";
            QString jsonStr = QJsonDocument(exitCmd).toJson(QJsonDocument::Compact);
            m_process->write(jsonStr.toUtf8() + "\n");
            m_process->waitForBytesWritten(1000);
            m_process->waitForFinished(3000);

            if (m_process->state() == QProcess::Running) {
                m_process->kill();
                m_process->waitForFinished(1000);
            }
        }

        m_process->deleteLater();
        m_process = nullptr;
        m_modelLoaded = false;

        emit logMessage("服务已停止");
        emit serviceStateChanged(false);
    }
}

bool DLService::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

QJsonObject DLService::sendRequest(const QJsonObject &request)
{
    if (!isRunning()) {
        return QJsonObject{{"success", false}, {"message", "服务未运行"}};
    }

    // 发送请求
    QString jsonStr = QJsonDocument(request).toJson(QJsonDocument::Compact);
    m_process->write(jsonStr.toUtf8() + "\n");

    if (!m_process->waitForBytesWritten(5000)) {
        return QJsonObject{{"success", false}, {"message", "发送请求超时"}};
    }

    // 等待响应
    if (!m_process->waitForReadyRead(30000)) {  // 30秒超时
        return QJsonObject{{"success", false}, {"message", "等待响应超时"}};
    }

    // 读取响应（循环读取直到获得有效 JSON）
    QString response;
    for (int i = 0; i < 10; ++i) {  // 最多尝试 10 次
        response = QString::fromUtf8(m_process->readLine()).trimmed();
        if (!response.isEmpty()) {
            break;
        }
        // 如果空行，等待更多数据
        if (!m_process->waitForReadyRead(1000)) {
            break;
        }
    }

    // 调试输出原始响应
    emit logMessage(QString("[调试] 原始响应: %1").arg(response));

    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());

    if (doc.isNull()) {
        return QJsonObject{{"success", false}, {"message", "无效的 JSON 响应: " + response}};
    }

    return doc.object();
}

bool DLService::loadModel(const QString &modelPath, const QString &labelsPath)
{
    QJsonObject request;
    request["command"] = "load_model";
    request["model_path"] = modelPath;

    if (!labelsPath.isEmpty()) {
        request["labels_path"] = labelsPath;
    }

    QJsonObject response = sendRequest(request);
    bool success = response["success"].toBool();

    if (success) {
        m_modelLoaded = true;
        m_modelPath = modelPath;
        emit logMessage("模型加载成功: " + modelPath);

        // 保存到缓存管理器
        EnvironmentCacheManager::instance()->setLastUsedModel(modelPath);
    } else {
        m_modelLoaded = false;
        emit logMessage("模型加载失败: " + response["message"].toString());
    }

    emit modelLoaded(success, response["message"].toString());
    return success;
}

DetectionResult DLService::parseDetectionResult(const QJsonObject &response)
{
    DetectionResult result;
    result.success = response["success"].toBool();
    result.message = response["message"].toString();

    if (result.success && response.contains("data")) {
        QJsonObject data = response["data"].toObject();
        QJsonArray detections = data["detections"].toArray();

        for (const QJsonValue &val : detections) {
            QJsonObject obj = val.toObject();
            Detection det;
            det.x = obj["x"].toInt();
            det.y = obj["y"].toInt();
            det.width = obj["width"].toInt();
            det.height = obj["height"].toInt();
            det.confidence = static_cast<float>(obj["confidence"].toDouble());
            det.classId = obj["class_id"].toInt();
            det.label = obj["label"].toString();

            // 解析掩码多边形
            if (obj.contains("mask_polygon")) {
                QJsonArray maskArray = obj["mask_polygon"].toArray();
                for (const QJsonValue &ptVal : maskArray) {
                    QJsonObject ptObj = ptVal.toObject();
                    MaskPoint pt;
                    pt.x = static_cast<float>(ptObj["x"].toDouble());
                    pt.y = static_cast<float>(ptObj["y"].toDouble());
                    det.maskPolygon.append(pt);
                }
            }

            result.detections.append(det);
        }
    }

    return result;
}

DetectionResult DLService::detect(const QString &imagePath,
                                         float confThreshold,
                                         float iouThreshold,
                                         int imageSize)
{
    if (!m_modelLoaded) {
        DetectionResult result;
        result.success = false;
        result.message = "模型未加载";
        emit logMessage(result.message);
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    QJsonObject request;
    request["command"] = "detect";
    request["image_path"] = imagePath;
    request["conf_threshold"] = confThreshold;
    request["iou_threshold"] = iouThreshold;
    request["image_size"] = imageSize;

    QJsonObject response = sendRequest(request);
    DetectionResult result = parseDetectionResult(response);
    result.inferenceTime = timer.elapsed();

    if (result.success) {
        emit logMessage(QString("检测完成: %1 个目标, 耗时 %2ms")
                        .arg(result.detections.size())
                        .arg(result.inferenceTime));
    }

    emit detectionCompleted(result);
    return result;
}

DetectionResult DLService::segment(const QString &imagePath,
                                          float confThreshold,
                                          float iouThreshold,
                                          int imageSize)
{
    if (!m_modelLoaded) {
        DetectionResult result;
        result.success = false;
        result.message = "模型未加载";
        emit logMessage(result.message);
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    QJsonObject request;
    request["command"] = "segment";
    request["image_path"] = imagePath;
    request["conf_threshold"] = confThreshold;
    request["iou_threshold"] = iouThreshold;
    request["image_size"] = imageSize;

    QJsonObject response = sendRequest(request);
    DetectionResult result = parseDetectionResult(response);
    result.inferenceTime = timer.elapsed();

    if (result.success) {
        emit logMessage(QString("分割完成: %1 个实例, 耗时 %2ms")
                        .arg(result.detections.size())
                        .arg(result.inferenceTime));
    }

    emit detectionCompleted(result);
    return result;
}

ClassificationResultList DLService::parseClassificationResult(const QJsonObject &response)
{
    ClassificationResultList result;
    result.success = response["success"].toBool();
    result.message = response["message"].toString();

    if (result.success && response.contains("data")) {
        QJsonObject data = response["data"].toObject();
        QJsonArray classifications = data["classifications"].toArray();

        for (const QJsonValue &val : classifications) {
            QJsonObject obj = val.toObject();
            ClassificationResult cls;
            cls.rank = obj["rank"].toInt();
            cls.confidence = static_cast<float>(obj["confidence"].toDouble());
            cls.classId = obj["class_id"].toInt();
            cls.label = obj["label"].toString();
            // FSL 特有字段
            if (obj.contains("avg_distance")) {
                cls.avgDistance = static_cast<float>(obj["avg_distance"].toDouble());
            }
            if (obj.contains("episode_count")) {
                cls.episodeCount = obj["episode_count"].toInt();
            }
            result.classifications.append(cls);
        }

        // Set top prediction
        if (!result.classifications.isEmpty()) {
            result.topPrediction = result.classifications.first();
        }
    }

    return result;
}

KeypointResult DLService::parseKeypointResult(const QJsonObject &response)
{
    KeypointResult result;
    result.success = response["success"].toBool();
    result.message = response["message"].toString();

    if (result.success && response.contains("data")) {
        QJsonObject data = response["data"].toObject();
        QJsonArray detections = data["detections"].toArray();

        for (const QJsonValue &val : detections) {
            QJsonObject obj = val.toObject();
            KeypointDetection det;
            det.x = obj["x"].toInt();
            det.y = obj["y"].toInt();
            det.width = obj["width"].toInt();
            det.height = obj["height"].toInt();
            det.confidence = static_cast<float>(obj["confidence"].toDouble());
            det.classId = obj["class_id"].toInt();
            det.label = obj["label"].toString();

            // Parse keypoints
            QJsonArray keypoints = obj["keypoints"].toArray();
            for (const QJsonValue &kpVal : keypoints) {
                QJsonObject kpObj = kpVal.toObject();
                KeypointData kp;
                kp.id = kpObj["id"].toInt();
                kp.x = static_cast<float>(kpObj["x"].toDouble());
                kp.y = static_cast<float>(kpObj["y"].toDouble());
                kp.confidence = static_cast<float>(kpObj["confidence"].toDouble(1.0));
                det.keypoints.append(kp);
            }

            result.detections.append(det);
        }
    }

    return result;
}

ClassificationResultList DLService::classify(const QString &imagePath, int topK)
{
    if (!m_modelLoaded) {
        ClassificationResultList result;
        result.success = false;
        result.message = "模型未加载";
        emit logMessage(result.message);
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    QJsonObject request;
    request["command"] = "classify";
    request["image_path"] = imagePath;
    request["top_k"] = topK;

    QJsonObject response = sendRequest(request);
    ClassificationResultList result = parseClassificationResult(response);
    result.inferenceTime = timer.elapsed();

    if (result.success) {
        emit logMessage(QString("分类完成: %1 (%2%), 耗时 %3ms")
                        .arg(result.topPrediction.label)
                        .arg(static_cast<int>(result.topPrediction.confidence * 100))
                        .arg(result.inferenceTime));
    }

    emit classificationCompleted(result);
    return result;
}

ClassificationResultList DLService::fewShotClassify(const QString &imagePath,
                                                    int nWay,
                                                    int nShot,
                                                    int nQuery,
                                                    int imageSize)
{
    if (!m_modelLoaded) {
        ClassificationResultList result;
        result.success = false;
        result.message = "模型未加载";
        emit logMessage(result.message);
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    QJsonObject request;
    request["command"] = "few_shot_classify";
    request["image_path"] = imagePath;
    request["n_way"] = nWay;
    request["n_shot"] = nShot;
    request["n_query"] = nQuery;
    request["image_size"] = imageSize;

    // 调试：输出发送的请求
    emit logMessage(QString("[调试] 发送请求: %1").arg(QJsonDocument(request).toJson(QJsonDocument::Compact)));

    QJsonObject response = sendRequest(request);
    ClassificationResultList result = parseClassificationResult(response);
    result.inferenceTime = timer.elapsed();

    if (result.success) {
        // 使用 QString::number 保留小数位，避免 static_cast<int> 截断小数值
        QString confidenceStr = QString::number(result.topPrediction.confidence * 100, 'f', 2);
        emit logMessage(QString("小样本分类完成: %1 (%2%), 耗时 %3ms")
                        .arg(result.topPrediction.label.isEmpty() ? "Unknown" : result.topPrediction.label)
                        .arg(confidenceStr)
                        .arg(result.inferenceTime));
    } else {
        emit logMessage(QString("小样本分类失败: %1").arg(result.message));
    }

    emit classificationCompleted(result);
    return result;
}

KeypointResult DLService::keypoint(const QString &imagePath,
                                          float confThreshold,
                                          float iouThreshold,
                                          int imageSize)
{
    if (!m_modelLoaded) {
        KeypointResult result;
        result.success = false;
        result.message = "模型未加载";
        emit logMessage(result.message);
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    QJsonObject request;
    request["command"] = "keypoint";
    request["image_path"] = imagePath;
    request["conf_threshold"] = confThreshold;
    request["iou_threshold"] = iouThreshold;
    request["image_size"] = imageSize;

    QJsonObject response = sendRequest(request);
    KeypointResult result = parseKeypointResult(response);
    result.inferenceTime = timer.elapsed();

    if (result.success) {
        emit logMessage(QString("关键点检测完成: %1 个目标, 耗时 %2ms")
                        .arg(result.detections.size())
                        .arg(result.inferenceTime));
    }

    emit keypointCompleted(result);
    return result;
}

} // namespace Utils
} // namespace GenPreCVSystem
