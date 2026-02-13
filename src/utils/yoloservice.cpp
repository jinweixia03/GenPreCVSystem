/**
 * @file yoloservice.cpp
 * @brief YOLO 推理服务实现
 *
 * 实现 YOLO 模型的 Python 后端通信，支持：
 * - 目标检测 (Object Detection)
 * - 实例分割 (Instance Segmentation)
 * - 图像分类 (Image Classification)
 * - 姿态/关键点检测 (Pose/Keypoint Detection)
 *
 * 通过 QProcess 与 Python 脚本通信，使用 JSON 格式传输数据
 */

#include "yoloservice.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QElapsedTimer>
#include <QSettings>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QTextStream>
#include <QSet>

namespace GenPreCVSystem {
namespace Utils {

// 默认的 conda 环境名称
static const QString DEFAULT_CONDA_ENV = "GenPreCVSystem";

YOLOService::YOLOService(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_modelLoaded(false)
{
}

YOLOService::~YOLOService()
{
    stop();
}

QVector<PythonEnvironment> YOLOService::scanEnvironments()
{
    QVector<PythonEnvironment> environments;
    QString homeDir = QDir::homePath();
    QSet<QString> addedPaths;  // 用于去重

    // 辅助函数：添加环境到列表
    auto addEnvironment = [&](const QString &name, const QString &pythonPath, const QString &type) {
        QString normalizedPath = QDir::cleanPath(pythonPath);
        if (addedPaths.contains(normalizedPath) || !QFile::exists(normalizedPath)) {
            return;
        }
        addedPaths.insert(normalizedPath);

        PythonEnvironment env;
        env.name = name;
        env.path = normalizedPath;
        env.type = type;

        // 检查是否有 ultralytics
        QProcess checkUltra;
        checkUltra.start(normalizedPath, QStringList() << "-c" << "import ultralytics; print('ok')");
        if (checkUltra.waitForFinished(5000) && checkUltra.exitCode() == 0) {
            env.hasUltralytics = true;
        } else {
            env.hasUltralytics = false;
        }
        environments.append(env);
    };

    // 1. 添加系统 Python
    QProcess sysPython;
    sysPython.start("python", QStringList() << "--version");
    if (sysPython.waitForFinished(3000) && sysPython.exitCode() == 0) {
        PythonEnvironment env;
        env.name = "系统 Python";
        env.path = "python";
        env.type = "system";

        // 检查是否有 ultralytics
        QProcess checkUltra;
        checkUltra.start("python", QStringList() << "-c" << "import ultralytics; print('ok')");
        if (checkUltra.waitForFinished(5000) && checkUltra.exitCode() == 0) {
            env.hasUltralytics = true;
        } else {
            env.hasUltralytics = false;
        }
        environments.append(env);
    }

    // 2. 读取 ~/.conda/environments.txt 获取所有 conda 环境路径（推荐方式）
    QString envFilePath = homeDir + "/.conda/environments.txt";
    QFile envFile(envFilePath);
    if (envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&envFile);
        while (!in.atEnd()) {
            QString envPath = in.readLine().trimmed();
            if (envPath.isEmpty()) continue;

            // 规范化路径（统一分隔符）
            QString normalizedPath = QDir::cleanPath(envPath);

            // 从路径中提取环境名称
            QString envName = QFileInfo(normalizedPath).fileName();
            if (envName.isEmpty()) continue;

            // 判断是否是 base 环境：检查父目录名是否为 "envs"
            QDir dir(normalizedPath);
            QString parentName = dir.cdUp() ? dir.dirName() : QString();

            if (parentName.compare("envs", Qt::CaseInsensitive) != 0) {
                // 父目录不是 envs，这是 conda 根目录（base 环境）
                envName = "base";
            }
            // 否则使用目录名作为环境名（已经是 envName）

            // 构建 Python 路径
            QString pythonPath;
#ifdef Q_OS_WIN
            pythonPath = normalizedPath + "/python.exe";
#else
            pythonPath = normalizedPath + "/bin/python";
#endif

            if (QFile::exists(pythonPath)) {
                addEnvironment(envName, pythonPath, "conda");
            }
        }
        envFile.close();
    }

    // 3. 使用 conda env list 获取所有 conda 环境
    QProcess condaProcess;
    condaProcess.start("conda", QStringList() << "env" << "list");
    if (condaProcess.waitForFinished(10000) && condaProcess.exitCode() == 0) {
        QString output = QString::fromUtf8(condaProcess.readAllStandardOutput());
        QStringList lines = output.split('\n');

        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith('#')) {
                continue;
            }

            // 格式: name path
            // 或者: name * path (当前激活的环境带 *)
            QRegularExpression regex(R"(^(\S+)\s+(\*?\s*)?(\S+)$)");
            QRegularExpressionMatch match = regex.match(trimmed);

            if (match.hasMatch()) {
                QString envName = match.captured(1);
                QString envPath = match.captured(3);

                // 构建 Python 路径
                QString pythonPath;
#ifdef Q_OS_WIN
                pythonPath = envPath + "/python.exe";
#else
                pythonPath = envPath + "/bin/python";
#endif

                if (QFile::exists(pythonPath)) {
                    addEnvironment(envName, pythonPath, "conda");
                }
            }
        }
    }

    // 4. 扫描常见的 conda 安装目录（作为备用）
    QStringList condaEnvDirs = {
        homeDir + "/miniconda3/envs",
        homeDir + "/anaconda3/envs",
        homeDir + "/Miniconda3/envs",
        homeDir + "/Anaconda3/envs",
        homeDir + "/.conda/envs",
        "C:/ProgramData/miniconda3/envs",
        "C:/ProgramData/anaconda3/envs",
        // 添加自定义安装路径
        "D:/Environments/MiniConda3/envs",
        "D:/Environments/Anaconda3/envs",
        "D:/Miniconda3/envs",
        "D:/Anaconda3/envs",
        "E:/Environments/MiniConda3/envs",
        "E:/Environments/Anaconda3/envs",
    };

    for (const QString &envDir : condaEnvDirs) {
        QDir dir(envDir);
        if (!dir.exists()) continue;

        QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subDir : subDirs) {
            QString pythonPath;
#ifdef Q_OS_WIN
            pythonPath = envDir + "/" + subDir + "/python.exe";
#else
            pythonPath = envDir + "/" + subDir + "/bin/python";
#endif
            addEnvironment(subDir, pythonPath, "conda");
        }
    }

    return environments;
}

QString YOLOService::currentEnvironmentPath() const
{
    return m_environmentPath;
}

void YOLOService::setEnvironmentPath(const QString &path)
{
    m_environmentPath = path;

    // 保存到配置文件
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configPath);
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }

    QSettings settings(configPath + "/yolo_settings.ini", QSettings::IniFormat);
    settings.setValue("python/path", path);
    settings.sync();
}

QString YOLOService::getDefaultScriptPath() const
{
    // 查找默认脚本路径
    QString appDir = QCoreApplication::applicationDirPath();
    QDir appQDir(appDir);

    // 尝试多个可能的路径
    QStringList possiblePaths = {
        appDir + "/../src/yolo_service.py",
        appDir + "/src/yolo_service.py",
        QCoreApplication::applicationDirPath() + "/../../src/yolo_service.py"
    };

    // 尝试从构建目录找到源目录
    QDir buildDir(appDir);
    if (buildDir.cdUp()) {
        possiblePaths.prepend(buildDir.absolutePath() + "/src/yolo_service.py");
    }

    for (const QString &path : possiblePaths) {
        QString normalized = QDir::cleanPath(path);
        if (QFile::exists(normalized)) {
            return normalized;
        }
    }

    // 默认返回第一个路径
    return QDir::cleanPath(possiblePaths.first());
}

QString YOLOService::findCondaPython() const
{
    // 1. 首先检查配置文件中是否有指定的 Python 路径
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configPath);
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }

    QSettings settings(configPath + "/yolo_settings.ini", QSettings::IniFormat);
    QString savedPythonPath = settings.value("python/path").toString();
    if (!savedPythonPath.isEmpty() && QFile::exists(savedPythonPath)) {
        return savedPythonPath;
    }

    // 2. 尝试使用 conda run 命令（推荐方式）
    // 检查 conda 是否可用
    QProcess testProcess;
    testProcess.start("conda", QStringList() << "--version");
    if (testProcess.waitForFinished(3000) && testProcess.exitCode() == 0) {
        // conda 可用，使用 conda run 方式
        return "conda";  // 返回特殊标记，表示使用 conda run
    }

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

bool YOLOService::start(const QString &pythonPath, const QString &scriptPath)
{
    if (m_process && m_process->state() == QProcess::Running) {
        emit logMessage("YOLO 服务已在运行中");
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

    // 设置进程通道模式
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    QStringList args;
    bool useCondaRun = false;

    if (python == "conda") {
        // 使用 conda run 方式运行
        useCondaRun = true;
        args << "run" << "-n" << DEFAULT_CONDA_ENV << "python" << script;
        emit logMessage(QString("启动 YOLO 服务: conda run -n %1 python %2").arg(DEFAULT_CONDA_ENV, script));
        m_process->start("conda", args);
    } else {
        // 直接使用 Python 路径
        args << script;
        emit logMessage(QString("启动 YOLO 服务: %1 %2").arg(python, script));
        m_process->start(python, args);
    }

    // 等待进程启动
    if (!m_process->waitForStarted(10000)) {
        QString errorMsg = m_process->errorString();
        emit logMessage("YOLO 服务启动失败: " + errorMsg);

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
        emit logMessage("YOLO 服务未响应 (可能 ultralytics 未安装)");
        stop();
        return false;
    }

    // 读取就绪响应
    QString response = QString::fromUtf8(m_process->readLine()).trimmed();
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    if (doc.isNull() || !doc.object()["success"].toBool()) {
        QString msg = doc.isNull() ? "无效响应" : doc.object()["message"].toString();
        emit logMessage("YOLO 服务初始化失败: " + msg);
        stop();
        return false;
    }

    emit logMessage("YOLO 服务启动成功");
    emit serviceStateChanged(true);
    return true;
}

void YOLOService::stop()
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

        emit logMessage("YOLO 服务已停止");
        emit serviceStateChanged(false);
    }
}

bool YOLOService::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

QJsonObject YOLOService::sendRequest(const QJsonObject &request)
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

    // 读取响应
    QString response = QString::fromUtf8(m_process->readLine()).trimmed();
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());

    if (doc.isNull()) {
        return QJsonObject{{"success", false}, {"message", "无效的 JSON 响应"}};
    }

    return doc.object();
}

bool YOLOService::loadModel(const QString &modelPath, const QString &labelsPath)
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
    } else {
        m_modelLoaded = false;
        emit logMessage("模型加载失败: " + response["message"].toString());
    }

    emit modelLoaded(success, response["message"].toString());
    return success;
}

YOLODetectionResult YOLOService::parseDetectionResult(const QJsonObject &response)
{
    YOLODetectionResult result;
    result.success = response["success"].toBool();
    result.message = response["message"].toString();

    if (result.success && response.contains("data")) {
        QJsonObject data = response["data"].toObject();
        QJsonArray detections = data["detections"].toArray();

        for (const QJsonValue &val : detections) {
            QJsonObject obj = val.toObject();
            YOLODetection det;
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

YOLODetectionResult YOLOService::detect(const QString &imagePath,
                                         float confThreshold,
                                         float iouThreshold,
                                         int imageSize)
{
    if (!m_modelLoaded) {
        YOLODetectionResult result;
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
    YOLODetectionResult result = parseDetectionResult(response);
    result.inferenceTime = timer.elapsed();

    if (result.success) {
        emit logMessage(QString("检测完成: %1 个目标, 耗时 %2ms")
                        .arg(result.detections.size())
                        .arg(result.inferenceTime));
    }

    emit detectionCompleted(result);
    return result;
}

YOLODetectionResult YOLOService::segment(const QString &imagePath,
                                          float confThreshold,
                                          float iouThreshold,
                                          int imageSize)
{
    if (!m_modelLoaded) {
        YOLODetectionResult result;
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
    YOLODetectionResult result = parseDetectionResult(response);
    result.inferenceTime = timer.elapsed();

    if (result.success) {
        emit logMessage(QString("分割完成: %1 个实例, 耗时 %2ms")
                        .arg(result.detections.size())
                        .arg(result.inferenceTime));
    }

    emit detectionCompleted(result);
    return result;
}

YOLOClassificationResult YOLOService::parseClassificationResult(const QJsonObject &response)
{
    YOLOClassificationResult result;
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
            result.classifications.append(cls);
        }

        // Set top prediction
        if (!result.classifications.isEmpty()) {
            result.topPrediction = result.classifications.first();
        }
    }

    return result;
}

YOLOKeypointResult YOLOService::parseKeypointResult(const QJsonObject &response)
{
    YOLOKeypointResult result;
    result.success = response["success"].toBool();
    result.message = response["message"].toString();

    if (result.success && response.contains("data")) {
        QJsonObject data = response["data"].toObject();
        QJsonArray detections = data["detections"].toArray();

        for (const QJsonValue &val : detections) {
            QJsonObject obj = val.toObject();
            YOLOKeypointDetection det;
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

YOLOClassificationResult YOLOService::classify(const QString &imagePath, int topK)
{
    if (!m_modelLoaded) {
        YOLOClassificationResult result;
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
    YOLOClassificationResult result = parseClassificationResult(response);
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

YOLOKeypointResult YOLOService::keypoint(const QString &imagePath,
                                          float confThreshold,
                                          float iouThreshold,
                                          int imageSize)
{
    if (!m_modelLoaded) {
        YOLOKeypointResult result;
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
    YOLOKeypointResult result = parseKeypointResult(response);
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
