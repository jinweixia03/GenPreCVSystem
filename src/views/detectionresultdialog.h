#ifndef DETECTIONRESULTDIALOG_H
#define DETECTIONRESULTDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QVector>
#include "../utils/yoloservice.h"
#include "../models/tasktypes.h"
#include "imageview.h"

class QPushButton;
class QLabel;
class QTextEdit;
class QTableWidget;
class QSplitter;

namespace GenPreCVSystem {
namespace Views {

/**
 * @brief 检测结果展示对话框
 *
 * 浮动窗口，根据不同任务类型显示不同的结果展示方式
 */
class DetectionResultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DetectionResultDialog(QWidget *parent = nullptr);
    ~DetectionResultDialog();

    /**
     * @brief 设置检测结果 (目标检测)
     * @param showLabels 是否显示标签
     */
    void setResult(const QString &imagePath, const Utils::YOLODetectionResult &result, bool showLabels = true);
    void setResult(const QPixmap &pixmap, const Utils::YOLODetectionResult &result, bool showLabels = true);

    /**
     * @brief 设置分割结果 (语义分割 - 蒙版显示)
     * @param pixmap 原始图像
     * @param result 检测结果（包含分割掩码）
     * @param maskAlpha 掩码透明度 (0-100)
     * @param showBoxes 是否显示边界框
     * @param showLabels 是否显示标签
     */
    void setSegmentationResult(const QPixmap &pixmap, const Utils::YOLODetectionResult &result,
                               int maskAlpha = 50, bool showBoxes = false, bool showLabels = true);

    /**
     * @brief 设置分类结果
     */
    void setClassificationResult(const QPixmap &pixmap, const Utils::YOLOClassificationResult &result);

    /**
     * @brief 设置关键点检测结果
     * @param pixmap 原始图像
     * @param result 检测结果
     * @param showBoxes 是否显示边界框
     * @param showLabels 是否显示标签
     */
    void setKeypointResult(const QPixmap &pixmap, const Utils::YOLOKeypointResult &result,
                           bool showBoxes = true, bool showLabels = true);

    /**
     * @brief 设置图像处理结果 (增强/去噪/边缘检测)
     */
    void setImageProcessResult(const QPixmap &originalPixmap, const QPixmap &processedPixmap,
                               const QString &processType, double processTime);

    /**
     * @brief 清除结果
     */
    void clearResult();

signals:
    void saveRequested();

private slots:
    void onSaveClicked();
    void onZoomIn();
    void onZoomOut();
    void onFitToWindow();
    void onActualSize();

private:
    void setupUI();
    void setupClassificationUI();
    void setupComparisonUI();
    void updateInfoPanel();
    void updateClassificationPanel();
    void updateComparisonPanel();
    DetectionOverlay convertToOverlay(const Utils::YOLODetection &det, int index);
    void drawKeypointSkeleton(QPainter &painter, const Utils::YOLOKeypointDetection &kp, const QColor &color);

    // 主界面控件
    ImageView *m_imageView;
    QLabel *m_lblInfo;
    QTextEdit *m_txtDetails;
    QPushButton *m_btnSave;
    QPushButton *m_btnZoomIn;
    QPushButton *m_btnZoomOut;
    QPushButton *m_btnFit;
    QPushButton *m_btnActualSize;
    QPushButton *m_btnClose;

    // 分类结果专用控件
    QWidget *m_classificationPanel;
    QTableWidget *m_classificationTable;
    QLabel *m_lblTopPrediction;
    QSplitter *m_contentSplitter;  // 用于图像和分类结果的分割器

    // 图像对比专用控件
    QWidget *m_comparisonPanel;
    ImageView *m_originalView;
    ImageView *m_processedView;
    QSplitter *m_comparisonSplitter;

    // 数据
    QPixmap m_originalPixmap;
    QPixmap m_processedPixmap;
    Utils::YOLODetectionResult m_currentResult;
    Utils::YOLOClassificationResult m_classificationResult;
    Utils::YOLOKeypointResult m_keypointResult;
    QString m_currentImagePath;
    Models::CVTask m_currentTaskType;
    QString m_processType;
};

} // namespace Views
} // namespace GenPreCVSystem

#endif // DETECTIONRESULTDIALOG_H
