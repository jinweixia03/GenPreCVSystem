#ifndef DLSERVICE_H
#define DLSERVICE_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>
#include "environmentcachemanager.h"

namespace GenPreCVSystem {
namespace Utils {

// PythonEnvironment 从 environmentcachemanager.h 导入

/**
 * @brief 掩码点数据
 */
struct MaskPoint {
    float x;
    float y;
};

/**
 * @brief 检测结果
 */
struct Detection {
    int x;
    int y;
    int width;
    int height;
    float confidence;
    int classId;
    QString label;
    QVector<MaskPoint> maskPolygon;  // 分割掩码多边形点
};

/**
 * @brief 检测结果列表
 */
struct DetectionResult {
    bool success = false;
    QString message;
    QVector<Detection> detections;
    double inferenceTime = 0.0;
};

/**
 * @brief 分类结果项
 */
struct ClassificationResult {
    int rank;
    float confidence;
    int classId;
    QString label;
};

/**
 * @brief 关键点数据
 */
struct KeypointData {
    int id;
    float x;
    float y;
    float confidence = 1.0f;
};

/**
 * @brief 带关键点的检测结果
 */
struct KeypointDetection {
    int x;
    int y;
    int width;
    int height;
    float confidence;
    int classId;
    QString label;
    QVector<KeypointData> keypoints;
};

/**
 * @brief 关键点检测结果
 */
struct KeypointResult {
    bool success = false;
    QString message;
    QVector<KeypointDetection> detections;
    double inferenceTime = 0.0;
};

/**
 * @brief 分类结果
 */
struct ClassificationResultList {
    bool success = false;
    QString message;
    QVector<ClassificationResult> classifications;
    ClassificationResult topPrediction;
    double inferenceTime = 0.0;
};

/**
 * @brief 深度学习推理服务
 *
 * 管理 Python 后端服务进程，通过 stdin/stdout 进行通信
 */
class DLService : public QObject
{
    Q_OBJECT

public:
    explicit DLService(QObject *parent = nullptr);
    ~DLService();

    /**
     * @brief 启动服务
     * @param pythonPath Python 可执行文件路径（可选，默认使用系统 Python）
     * @param scriptPath 脚本路径（可选，默认使用内置路径）
     * @return 是否启动成功
     */
    bool start(const QString &pythonPath = QString(), const QString &scriptPath = QString());

    /**
     * @brief 停止服务
     */
    void stop();

    /**
     * @brief 检查服务是否正在运行
     */
    bool isRunning() const;

    /**
     * @brief 检查模型是否已加载
     */
    bool isModelLoaded() const { return m_modelLoaded; }

    /**
     * @brief 获取当前加载的模型路径
     */
    QString modelPath() const { return m_modelPath; }

    /**
     * @brief 扫描系统中所有可用的 Python 环境
     * @return 环境列表
     */
    static QVector<PythonEnvironment> scanEnvironments();

    /**
     * @brief 快速获取环境列表（优先使用缓存）
     * @return 环境列表（可能来自缓存）
     */
    static QVector<PythonEnvironment> getEnvironments();

    /**
     * @brief 验证指定环境是否安装了 ultralytics
     * @param env 要验证的环境
     * @return 是否已安装
     */
    static bool checkEnvironmentCapability(PythonEnvironment &env);

    /**
     * @brief 异步刷新环境缓存（后台线程执行）
     */
    static void refreshEnvironmentCacheAsync();

    /**
     * @brief 获取当前选中的环境路径
     */
    QString currentEnvironmentPath() const;

    /**
     * @brief 设置要使用的 Python 环境路径
     */
    void setEnvironmentPath(const QString &path);

    /**
     * @brief 快速启动服务（使用缓存验证）
     * @return 是否启动成功
     */
    bool fastStart();

    /**
     * @brief 检查指定环境是否可以直接启动（使用缓存）
     * @param envPath Python 可执行文件路径
     * @return 是否可以直接启动
     */
    bool canFastStart(const QString &envPath) const;

    /**
     * @brief 获取上次使用的环境路径（从缓存）
     * @return 环境路径
     */
    QString getLastUsedEnvironment() const;

    /**
     * @brief 获取上次使用的模型路径（从缓存）
     * @return 模型路径
     */
    QString getLastUsedModel() const;

    /**
     * @brief 加载模型
     * @param modelPath 模型文件路径
     * @param labelsPath 标签文件路径（可选）
     * @return 是否加载成功
     */
    bool loadModel(const QString &modelPath, const QString &labelsPath = QString());

    /**
     * @brief 执行目标检测
     * @param imagePath 图像文件路径
     * @param confThreshold 置信度阈值
     * @param iouThreshold IOU 阈值
     * @param imageSize 输入图像尺寸
     * @return 检测结果
     */
    DetectionResult detect(const QString &imagePath,
                                float confThreshold = 0.25f,
                                float iouThreshold = 0.45f,
                                int imageSize = 640);

    /**
     * @brief 执行实例分割
     * @param imagePath 图像文件路径
     * @param confThreshold 置信度阈值
     * @param iouThreshold IOU 阈值
     * @param imageSize 输入图像尺寸
     * @return 分割结果
     */
    DetectionResult segment(const QString &imagePath,
                                 float confThreshold = 0.25f,
                                 float iouThreshold = 0.45f,
                                 int imageSize = 640);

    /**
     * @brief 执行图像分类
     * @param imagePath 图像文件路径
     * @param topK 返回的 top-k 结果数量
     * @return 分类结果
     */
    ClassificationResultList classify(const QString &imagePath, int topK = 5);

    /**
     * @brief 执行关键点/姿态检测
     * @param imagePath 图像文件路径
     * @param confThreshold 置信度阈值
     * @param iouThreshold IOU 阈值
     * @param imageSize 输入图像尺寸
     * @return 关键点检测结果
     */
    KeypointResult keypoint(const QString &imagePath,
                                 float confThreshold = 0.25f,
                                 float iouThreshold = 0.45f,
                                 int imageSize = 640);

signals:
    /**
     * @brief 服务状态改变信号
     */
    void serviceStateChanged(bool running);

    /**
     * @brief 模型加载完成信号
     */
    void modelLoaded(bool success, const QString &message);

    /**
     * @brief 检测完成信号
     */
    void detectionCompleted(const DetectionResult &result);

    /**
     * @brief 分类完成信号
     */
    void classificationCompleted(const ClassificationResultList &result);

    /**
     * @brief 关键点检测完成信号
     */
    void keypointCompleted(const KeypointResult &result);

    /**
     * @brief 日志消息信号
     */
    void logMessage(const QString &message);

private:
    /**
     * @brief 发送请求并等待响应
     */
    QJsonObject sendRequest(const QJsonObject &request);

    /**
     * @brief 解析检测结果
     */
    DetectionResult parseDetectionResult(const QJsonObject &response);

    /**
     * @brief 解析分类结果
     */
    ClassificationResultList parseClassificationResult(const QJsonObject &response);

    /**
     * @brief 解析关键点检测结果
     */
    KeypointResult parseKeypointResult(const QJsonObject &response);

    /**
     * @brief 获取默认脚本路径
     */
    QString getDefaultScriptPath() const;

    /**
     * @brief 查找 conda 环境中的 Python
     * @return Python 可执行文件路径，或 "conda" 表示使用 conda run
     */
    QString findCondaPython() const;

    QProcess *m_process;
    bool m_modelLoaded;
    QString m_modelPath;
    QString m_environmentPath;  // 当前选中的环境路径
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // DLSERVICE_H
