#ifndef TASKCONTROLLER_H
#define TASKCONTROLLER_H

#include <QObject>
#include <QActionGroup>
#include <QScrollArea>
#include <memory>
#include <functional>

#include "../models/tasktypes.h"

// 前向声明
class QTabWidget;
class ImageView;  // 使用全局命名空间的 ImageView（定义在 mainwindow.h 中）

namespace GenPreCVSystem {
namespace Views {
class DetectionResultDialog;
}
namespace Utils {
class YOLOService;
class ImageProcessService;
struct YOLODetectionResult;
struct YOLODetection;
struct YOLOClassificationResult;
struct YOLOKeypointResult;
struct ProcessResult;
}
namespace Controllers {
class TabController;
}
namespace Widgets {
class EnvironmentServiceWidget;
}
}

namespace GenPreCVSystem {
namespace Controllers {

/**
 * @brief 任务控制器
 *
 * 管理CV任务类型的切换和参数面板更新
 */
class TaskController : public QObject
{
    Q_OBJECT

public:
    explicit TaskController(QObject *parent = nullptr);
    ~TaskController();

    /**
     * @brief 设置参数面板滚动区域
     */
    void setParameterScrollArea(QScrollArea *scrollArea);

    /**
     * @brief 设置任务动作组
     */
    void setTaskActionGroup(QActionGroup *actionGroup);

    /**
     * @brief 设置标签页控制器
     */
    void setTabController(TabController *tabController);

    /**
     * @brief 设置标签页组件（用于直接获取当前 ImageView）
     */
    void setTabWidget(QTabWidget *tabWidget);

    /**
     * @brief 切换任务类型
     */
    void switchTask(Models::CVTask task);

    /**
     * @brief 获取当前任务类型
     */
    Models::CVTask currentTask() const { return m_currentTask; }

    /**
     * @brief 获取 YOLO 服务
     */
    Utils::YOLOService* yoloService() const { return m_yoloService; }

    /**
     * @brief 设置是否显示结果对话框
     * @param show true 显示结果对话框，false 不显示
     *
     * 用于批量处理时禁用结果对话框显示
     */
    void setShowResultDialog(bool show) { m_showResultDialog = show; }

    /**
     * @brief 获取是否显示结果对话框
     */
    bool showResultDialogEnabled() const { return m_showResultDialog; }

    /**
     * @brief 扫描指定任务类型的可用模型
     * @param task 任务类型
     * @return 模型文件路径列表
     */
    static QStringList scanAvailableModels(Models::CVTask task);

    /**
     * @brief 获取模型目录路径
     * @param task 任务类型
     * @return 模型目录路径
     */
    static QString getModelDirectory(Models::CVTask task);

signals:
    /**
     * @brief 任务已切换信号
     */
    void taskChanged(Models::CVTask task);

    /**
     * @brief 日志消息信号
     */
    void logMessage(const QString &message);

    /**
     * @brief 检测完成信号
     */
    void detectionCompleted(const Utils::YOLODetectionResult &result);

    /**
     * @brief 图像处理完成信号
     */
    void imageProcessCompleted(const Utils::ProcessResult &result);

public slots:
    /**
     * @brief 启动 YOLO 服务
     */
    bool startYOLOService();

    /**
     * @brief 停止 YOLO 服务
     */
    void stopYOLOService();

    /**
     * @brief 加载 YOLO 模型
     */
    bool loadYOLOModel(const QString &modelPath, const QString &labelsPath = QString());

    /**
     * @brief 设置当前图像路径（由 MainWindow 调用）
     */
    void setCurrentImagePath(const QString &imagePath);

    /**
     * @brief 设置当前显示的图像（用于结果显示）
     */
    void setCurrentPixmap(const QPixmap &pixmap);

    /**
     * @brief 执行目标检测
     */
    void runDetection(const QString &imagePath, float confThreshold = 0.25f,
                      float iouThreshold = 0.45f, int imageSize = 640);

    /**
     * @brief 执行实例分割
     */
    void runSegmentation(const QString &imagePath, float confThreshold = 0.25f,
                         float iouThreshold = 0.45f, int imageSize = 640);

    /**
     * @brief 执行图像分类
     */
    void runClassification(const QString &imagePath, int topK = 5);

    /**
     * @brief 执行关键点检测
     */
    void runKeypointDetection(const QString &imagePath, float confThreshold = 0.25f,
                               float iouThreshold = 0.45f, int imageSize = 640);

    /**
     * @brief 执行图像增强
     */
    void runImageEnhancement(const QString &imagePath, int brightness, int contrast,
                              int saturation, int sharpness);

    /**
     * @brief 执行图像去噪
     */
    void runImageDenoising(const QString &imagePath, int method, int kernelSize, double sigma);

    /**
     * @brief 执行边缘检测
     */
    void runEdgeDetection(const QString &imagePath, int method, double threshold1,
                           double threshold2, int apertureSize);

private slots:
    /**
     * @brief 处理检测结果并显示结果对话框
     */
    void onDetectionCompleted(const Utils::YOLODetectionResult &result);

private:
    void updateParameterPanel(Models::CVTask task);
    void clearParameterPanel();
    void connectParameterPanelSignals();
    void connectSharedWidgetSignals();
    void enableRunButtons(bool enabled);
    ::ImageView* getCurrentImageView() const;  // 获取当前 ImageView（全局命名空间）
    QString getCurrentImagePath() const;
    QString getCurrentImageForInference();  // 获取用于推理的图像路径（可能是临时文件）
    void cleanupTempImage();  // 清理临时图像文件
    void showResultDialog(const Utils::YOLODetectionResult &result);
    bool isAITask(Models::CVTask task) const;

    QScrollArea *m_paramScrollArea;
    QActionGroup *m_taskActionGroup;
    Models::CVTask m_currentTask;

    // YOLO 服务
    Utils::YOLOService *m_yoloService;

    // 图像处理服务
    Utils::ImageProcessService *m_imageProcessService;

    // 标签页控制器（用于获取当前图像路径）
    TabController *m_tabController;

    // 标签页组件（用于直接获取当前 ImageView）
    QTabWidget *m_tabWidget;

    // 当前图像路径（由 MainWindow 设置）
    QString m_currentImagePath;

    // 当前图像（用于显示结果）
    QPixmap m_currentPixmap;

    // 当前模型路径
    QString m_currentModelPath;

    // 临时图像文件路径（用于推理）
    QString m_tempImagePath;

    // 检测结果对话框
    Views::DetectionResultDialog *m_resultDialog;

    // 共享的环境服务控件
    Widgets::EnvironmentServiceWidget *m_envServiceWidget;

    // 任务特定参数面板容器
    QWidget *m_taskParamContainer;

    // 分割显示设置
    int m_maskAlpha;
    bool m_showBoxes;
    bool m_showLabels;

    // 控制是否显示结果对话框（批量处理时禁用）
    bool m_showResultDialog = true;
};

} // namespace Controllers
} // namespace GenPreCVSystem

#endif // TASKCONTROLLER_H
