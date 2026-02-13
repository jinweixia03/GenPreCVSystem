#ifndef BATCHPROCESSDIALOG_H
#define BATCHPROCESSDIALOG_H

#include <QDialog>
#include <QStringList>
#include "../utils/yoloservice.h"
#include "../models/tasktypes.h"

class QLineEdit;
class QLabel;
class QPushButton;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QProgressBar;
class QCheckBox;

namespace GenPreCVSystem {
namespace Utils {
class YOLOService;
}

namespace Views {

/**
 * @brief 批量处理对话框
 *
 * 支持对文件夹中的图像进行批量推理处理，导出为 ZIP 格式（包含 images 和 labels 文件夹）
 */
class BatchProcessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchProcessDialog(QWidget *parent = nullptr);
    ~BatchProcessDialog();

    /**
     * @brief 设置 YOLO 服务
     */
    void setYOLOService(Utils::YOLOService *service);

signals:
    /**
     * @brief 批处理完成信号
     * @param zipPath 导出的 ZIP 文件路径
     */
    void batchCompleted(const QString &zipPath);

private slots:
    void onBrowseFolder();
    void onBrowseModel();
    void onTaskTypeChanged(int index);
    void onModelSelectionChanged(int index);
    void onStartProcessing();
    void onStopProcessing();
    void onExportResults();
    void onClose();

private:
    void setupUI();
    void applyStyles();
    void updateModelList();
    void tryAutoLoadFirstModel();
    void populateImageList(const QString &folderPath);
    void processNextImage();
    void updateProgress();
    void finishProcessing();
    bool exportAsZip(const QString &zipPath);
    QString getModelDirectory() const;

    // 任务和模型选择
    QComboBox *m_comboTaskType;
    QComboBox *m_comboModel;
    QPushButton *m_btnBrowseModel;
    QLabel *m_lblModelStatus;

    // 参数控件
    QDoubleSpinBox *m_spinConfThreshold;
    QDoubleSpinBox *m_spinIOUThreshold;
    QSpinBox *m_spinImageSize;

    // 输入控件
    QLineEdit *m_editFolder;
    QPushButton *m_btnBrowse;
    QCheckBox *m_chkRecursive;
    QComboBox *m_comboImageFormat;

    // 进度控件
    QProgressBar *m_progressBar;
    QLabel *m_lblStatus;
    QLabel *m_lblProgress;

    // 按钮
    QPushButton *m_btnStart;
    QPushButton *m_btnStop;
    QPushButton *m_btnExport;
    QPushButton *m_btnClose;

    // 状态
    Utils::YOLOService *m_yoloService;
    Models::CVTask m_taskType;
    QString m_currentModelPath;
    QString m_currentFolder;
    QStringList m_imageFiles;
    int m_currentIndex;
    bool m_isProcessing;
    bool m_stopRequested;

    // 结果存储（检测/分割）
    QVector<QPair<QString, Utils::YOLODetectionResult>> m_detectionResults;
    // 分类结果存储
    QVector<QPair<QString, Utils::YOLOClassificationResult>> m_classificationResults;
    // 关键点结果存储
    QVector<QPair<QString, Utils::YOLOKeypointResult>> m_keypointResults;

    // 统计
    int m_successCount;
    int m_failCount;
    double m_totalTime;
};

} // namespace Views
} // namespace GenPreCVSystem

#endif // BATCHPROCESSDIALOG_H
