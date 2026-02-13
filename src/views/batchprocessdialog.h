#ifndef BATCHPROCESSDIALOG_H
#define BATCHPROCESSDIALOG_H

#include <QDialog>
#include <QStringList>
#include "../utils/yoloservice.h"
#include "../models/tasktypes.h"

class QLineEdit;
class QLabel;
class QTextEdit;
class QPushButton;
class QTableWidget;
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
 * 支持对文件夹中的图像进行批量推理处理
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

    /**
     * @brief 设置任务类型
     */
    void setTaskType(Models::CVTask task);

    /**
     * @brief 设置模型路径
     */
    void setModelPath(const QString &path);

signals:
    /**
     * @brief 批处理完成信号
     * @param reportPath 报告文件路径
     */
    void batchCompleted(const QString &reportPath);

private slots:
    void onBrowseFolder();
    void onStartProcessing();
    void onStopProcessing();
    void onExportResults();
    void onClose();

private:
    void setupUI();
    void applyStyles();
    void populateImageList(const QString &folderPath);
    void processNextImage();
    void updateProgress();
    void finishProcessing();
    void addLogEntry(const QString &message);
    void addResultRow(const QString &fileName, int detectionCount, double time, const QString &status);

    // 输入控件
    QLineEdit *m_editFolder;
    QPushButton *m_btnBrowse;
    QCheckBox *m_chkRecursive;
    QComboBox *m_comboImageFormat;

    // 参数控件
    QDoubleSpinBox *m_spinConfThreshold;
    QDoubleSpinBox *m_spinIOUThreshold;
    QSpinBox *m_spinImageSize;

    // 进度控件
    QProgressBar *m_progressBar;
    QLabel *m_lblStatus;
    QLabel *m_lblProgress;
    QTextEdit *m_txtLog;

    // 结果表格
    QTableWidget *m_resultsTable;

    // 按钮
    QPushButton *m_btnStart;
    QPushButton *m_btnStop;
    QPushButton *m_btnExport;
    QPushButton *m_btnClose;

    // 状态
    Utils::YOLOService *m_yoloService;
    Models::CVTask m_taskType;
    QString m_modelPath;
    QString m_currentFolder;
    QStringList m_imageFiles;
    int m_currentIndex;
    bool m_isProcessing;
    bool m_stopRequested;

    // 结果存储
    QVector<QPair<QString, Utils::YOLODetectionResult>> m_results;

    // 统计
    int m_successCount;
    int m_failCount;
    double m_totalTime;
};

} // namespace Views
} // namespace GenPreCVSystem

#endif // BATCHPROCESSDIALOG_H
