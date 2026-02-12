#ifndef YOLOSERVICE_H
#define YOLOSERVICE_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief Python 环境信息
 */
struct PythonEnvironment {
    QString name;           // 环境名称
    QString path;           // Python 可执行文件路径
    QString type;           // 环境类型: "conda", "venv", "system"
    bool hasUltralytics;    // 是否安装了 ultralytics
};

/**
 * @brief 掩码点数据
 */
struct MaskPoint {
    float x;
    float y;
};

/**
 * @brief YOLO 检测结果
 */
struct YOLODetection {
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
 * @brief YOLO 检测结果列表
 */
struct YOLODetectionResult {
    bool success = false;
    QString message;
    QVector<YOLODetection> detections;
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
struct YOLOKeypointDetection {
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
struct YOLOKeypointResult {
    bool success = false;
    QString message;
    QVector<YOLOKeypointDetection> detections;
    double inferenceTime = 0.0;
};

/**
 * @brief 分类结果
 */
struct YOLOClassificationResult {
    bool success = false;
    QString message;
    QVector<ClassificationResult> classifications;
    ClassificationResult topPrediction;
    double inferenceTime = 0.0;
};

/**
 * @brief YOLO 推理服务
 *
 * 管理 Python YOLO 服务进程，通过 stdin/stdout 进行通信
 */
class YOLOService : public QObject
{
    Q_OBJECT

public:
    explicit YOLOService(QObject *parent = nullptr);
    ~YOLOService();

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
     * @brief 扫描系统中所有可用的 Python 环境
     * @return 环境列表
     */
    static QVector<PythonEnvironment> scanEnvironments();

    /**
     * @brief 获取当前选中的环境路径
     */
    QString currentEnvironmentPath() const;

    /**
     * @brief 设置要使用的 Python 环境路径
     */
    void setEnvironmentPath(const QString &path);

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
    YOLODetectionResult detect(const QString &imagePath,
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
    YOLODetectionResult segment(const QString &imagePath,
                                 float confThreshold = 0.25f,
                                 float iouThreshold = 0.45f,
                                 int imageSize = 640);

    /**
     * @brief 执行图像分类
     * @param imagePath 图像文件路径
     * @param topK 返回的 top-k 结果数量
     * @return 分类结果
     */
    YOLOClassificationResult classify(const QString &imagePath, int topK = 5);

    /**
     * @brief 执行关键点/姿态检测
     * @param imagePath 图像文件路径
     * @param confThreshold 置信度阈值
     * @param iouThreshold IOU 阈值
     * @param imageSize 输入图像尺寸
     * @return 关键点检测结果
     */
    YOLOKeypointResult keypoint(const QString &imagePath,
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
    void detectionCompleted(const YOLODetectionResult &result);

    /**
     * @brief 分类完成信号
     */
    void classificationCompleted(const YOLOClassificationResult &result);

    /**
     * @brief 关键点检测完成信号
     */
    void keypointCompleted(const YOLOKeypointResult &result);

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
    YOLODetectionResult parseDetectionResult(const QJsonObject &response);

    /**
     * @brief 解析分类结果
     */
    YOLOClassificationResult parseClassificationResult(const QJsonObject &response);

    /**
     * @brief 解析关键点检测结果
     */
    YOLOKeypointResult parseKeypointResult(const QJsonObject &response);

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

#endif // YOLOSERVICE_H
