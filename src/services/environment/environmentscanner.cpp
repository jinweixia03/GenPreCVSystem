/**
 * @file environmentscanner.cpp
 * @brief 环境扫描器实现
 *
 * 使用简单的同步扫描，避免复杂的线程和事件循环问题。
 */

#include "environmentscanner.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTextStream>
#include <QDebug>

namespace GenPreCVSystem {
namespace Utils {

EnvironmentScanner::EnvironmentScanner(QObject *parent)
    : QObject(parent)
    , m_shouldStop(false)
    , m_isScanning(false)
{
}

EnvironmentScanner::~EnvironmentScanner()
{
    cancel();
}

QVector<CachedEnvironment> EnvironmentScanner::scan()
{
    m_shouldStop = false;
    m_isScanning = true;

    emit scanProgress("开始扫描 Python 环境...");

    try {
        QVector<PythonEnvironment> envs = doScan();

        // 转换为 CachedEnvironment
        QVector<CachedEnvironment> cachedList;
        for (const auto &env : envs) {
            cachedList.append(CachedEnvironment(env));
        }

        m_isScanning = false;

        emit scanProgress(QString("扫描完成，找到 %1 个环境").arg(envs.size()));
        emit scanCompleted(cachedList);

        return cachedList;
    } catch (const std::exception &e) {
        m_isScanning = false;
        QString errorMsg = QString("扫描异常: %1").arg(e.what());
        emit scanProgress(errorMsg);
        emit scanFailed(errorMsg);
        return QVector<CachedEnvironment>();
    } catch (...) {
        m_isScanning = false;
        QString errorMsg = "扫描发生未知异常";
        emit scanProgress(errorMsg);
        emit scanFailed(errorMsg);
        return QVector<CachedEnvironment>();
    }
}

void EnvironmentScanner::cancel()
{
    m_shouldStop = true;
}

QVector<PythonEnvironment> EnvironmentScanner::doScan()
{
    QVector<PythonEnvironment> environments;
    QString homeDir = QDir::homePath();
    QSet<QString> addedPaths;

    // 1. 读取 ~/.conda/environments.txt（最可靠且快速的方法）
    if (!m_shouldStop) {
        emit scanProgress("读取 Conda 环境列表...");

        QString envFilePath = homeDir + "/.conda/environments.txt";
        QFile envFile(envFilePath);
        if (envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&envFile);
            while (!in.atEnd() && !m_shouldStop) {
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

                QString normalizedPythonPath = QDir::cleanPath(pythonPath);
                if (QFile::exists(normalizedPythonPath) && !addedPaths.contains(normalizedPythonPath)) {
                    PythonEnvironment env;
                    env.name = envName;
                    env.path = normalizedPythonPath;
                    env.type = "conda";
                    env.hasUltralytics = false;
                    environments.append(env);
                    addedPaths.insert(normalizedPythonPath);
                }
            }
            envFile.close();
        }
    }

    // 2. 扫描常见的 conda 安装目录
    if (!m_shouldStop) {
        emit scanProgress("扫描常见 Conda 安装目录...");

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
        const int MAX_SCAN = 20;

        for (const QString &envDir : condaEnvDirs) {
            if (scannedCount >= MAX_SCAN || m_shouldStop) break;

            QDir dir(envDir);
            if (!dir.exists()) continue;

            QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &subDir : subDirs) {
                if (scannedCount >= MAX_SCAN || m_shouldStop) break;

                QString pythonPath;
#ifdef Q_OS_WIN
                pythonPath = envDir + "/" + subDir + "/python.exe";
#else
                pythonPath = envDir + "/" + subDir + "/bin/python";
#endif

                QString normalizedPythonPath = QDir::cleanPath(pythonPath);
                if (QFile::exists(normalizedPythonPath) && !addedPaths.contains(normalizedPythonPath)) {
                    PythonEnvironment env;
                    env.name = subDir;
                    env.path = normalizedPythonPath;
                    env.type = "conda";
                    env.hasUltralytics = false;
                    environments.append(env);
                    addedPaths.insert(normalizedPythonPath);
                    scannedCount++;
                }
            }
        }
    }

    // 3. 添加系统 Python
    if (!m_shouldStop) {
        emit scanProgress("检测系统 Python...");

        QStringList systemPythonPaths = {
            "python",
            "python3",
        };

        for (const QString &pyPath : systemPythonPaths) {
            if (addedPaths.contains(pyPath)) continue;

            PythonEnvironment env;
            env.name = "系统 Python";
            env.path = pyPath;
            env.type = "system";
            env.hasUltralytics = false;
            environments.append(env);
            addedPaths.insert(pyPath);
        }
    }

    return environments;
}

} // namespace Utils
} // namespace GenPreCVSystem
