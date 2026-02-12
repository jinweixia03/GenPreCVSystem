#include "detectionresultdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QColor>
#include <QPainter>
#include <QFileInfo>
#include <QFontMetrics>
#include <QTableWidget>
#include <QProgressBar>
#include <QSplitter>
#include <QStackedWidget>

namespace GenPreCVSystem {
namespace Views {

DetectionResultDialog::DetectionResultDialog(QWidget *parent)
    : QDialog(parent)
    , m_imageView(nullptr)
    , m_lblInfo(nullptr)
    , m_txtDetails(nullptr)
    , m_btnSave(nullptr)
    , m_btnZoomIn(nullptr)
    , m_btnZoomOut(nullptr)
    , m_btnFit(nullptr)
    , m_btnActualSize(nullptr)
    , m_btnClose(nullptr)
    , m_classificationPanel(nullptr)
    , m_classificationTable(nullptr)
    , m_lblTopPrediction(nullptr)
    , m_comparisonPanel(nullptr)
    , m_originalView(nullptr)
    , m_processedView(nullptr)
    , m_comparisonSplitter(nullptr)
    , m_currentTaskType(Models::CVTask::ObjectDetection)
{
    setupUI();
    setupClassificationUI();
    setupComparisonUI();
    setWindowTitle(tr("结果查看器"));
    resize(1000, 750);
}

DetectionResultDialog::~DetectionResultDialog()
{
}

void DetectionResultDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 顶部信息栏
    m_lblInfo = new QLabel(tr("暂无结果"), this);
    m_lblInfo->setStyleSheet(
        "QLabel { "
        "  background-color: #2d2d2d; "
        "  color: #ffffff; "
        "  padding: 8px; "
        "  border-radius: 4px; "
        "  font-size: 13px; "
        "}"
    );
    m_lblInfo->setWordWrap(true);
    mainLayout->addWidget(m_lblInfo);

    // 创建内容分割器（用于图像和分类结果）
    m_contentSplitter = new QSplitter(Qt::Horizontal, this);

    // 图像显示区域
    m_imageView = new ImageView(this);
    m_imageView->setMinimumSize(400, 300);
    m_imageView->setStyleSheet(
        "ImageView { "
        "  border: 2px solid #3d3d3d; "
        "  border-radius: 4px; "
        "}"
    );
    m_contentSplitter->addWidget(m_imageView);

    // 分类面板（初始不可见）
    m_classificationPanel = new QWidget(this);
    m_classificationPanel->setVisible(false);
    m_contentSplitter->addWidget(m_classificationPanel);

    // 设置分割器初始大小比例
    m_contentSplitter->setSizes({700, 300});

    mainLayout->addWidget(m_contentSplitter, 1);

    // 详细信息区域
    m_txtDetails = new QTextEdit(this);
    m_txtDetails->setReadOnly(true);
    m_txtDetails->setMaximumHeight(120);
    m_txtDetails->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1e1e1e; "
        "  color: #d4d4d4; "
        "  border: 1px solid #3d3d3d; "
        "  border-radius: 4px; "
        "  font-family: Consolas, 'Courier New', monospace; "
        "  font-size: 12px; "
        "}"
    );
    m_txtDetails->setVisible(false);
    mainLayout->addWidget(m_txtDetails);

    // 工具栏按钮
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(6);

    m_btnZoomIn = new QPushButton(tr("放大"), this);
    m_btnZoomIn->setFixedWidth(70);

    m_btnZoomOut = new QPushButton(tr("缩小"), this);
    m_btnZoomOut->setFixedWidth(70);

    m_btnFit = new QPushButton(tr("适应窗口"), this);
    m_btnFit->setFixedWidth(80);

    m_btnActualSize = new QPushButton(tr("实际大小"), this);
    m_btnActualSize->setFixedWidth(80);

    toolbarLayout->addWidget(m_btnZoomIn);
    toolbarLayout->addWidget(m_btnZoomOut);
    toolbarLayout->addWidget(m_btnFit);
    toolbarLayout->addWidget(m_btnActualSize);
    toolbarLayout->addStretch();

    QString btnStyle =
        "QPushButton { "
        "  background-color: #3d3d3d; "
        "  color: #ffffff; "
        "  border: none; "
        "  padding: 6px 12px; "
        "  border-radius: 4px; "
        "  font-size: 12px; "
        "} "
        "QPushButton:hover { "
        "  background-color: #4d4d4d; "
        "} "
        "QPushButton:pressed { "
        "  background-color: #2d2d2d; "
        "}";
    m_btnZoomIn->setStyleSheet(btnStyle);
    m_btnZoomOut->setStyleSheet(btnStyle);
    m_btnFit->setStyleSheet(btnStyle);
    m_btnActualSize->setStyleSheet(btnStyle);

    connect(m_btnZoomIn, &QPushButton::clicked, this, &DetectionResultDialog::onZoomIn);
    connect(m_btnZoomOut, &QPushButton::clicked, this, &DetectionResultDialog::onZoomOut);
    connect(m_btnFit, &QPushButton::clicked, this, &DetectionResultDialog::onFitToWindow);
    connect(m_btnActualSize, &QPushButton::clicked, this, &DetectionResultDialog::onActualSize);

    mainLayout->addLayout(toolbarLayout);

    // 底部按钮
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    m_btnSave = new QPushButton(tr("保存结果图像"), this);
    m_btnSave->setFixedWidth(120);
    m_btnSave->setStyleSheet(
        "QPushButton { "
        "  background-color: #0078d4; "
        "  color: #ffffff; "
        "  border: none; "
        "  padding: 8px 16px; "
        "  border-radius: 4px; "
        "  font-size: 13px; "
        "} "
        "QPushButton:hover { "
        "  background-color: #1a86d9; "
        "} "
        "QPushButton:pressed { "
        "  background-color: #006cbd; "
        "}"
    );

    m_btnClose = new QPushButton(tr("关闭"), this);
    m_btnClose->setFixedWidth(80);
    m_btnClose->setStyleSheet(
        "QPushButton { "
        "  background-color: #5d5d5d; "
        "  color: #ffffff; "
        "  border: none; "
        "  padding: 8px 16px; "
        "  border-radius: 4px; "
        "  font-size: 13px; "
        "} "
        "QPushButton:hover { "
        "  background-color: #6d6d6d; "
        "} "
        "QPushButton:pressed { "
        "  background-color: #4d4d4d; "
        "}"
    );

    bottomLayout->addWidget(m_btnSave);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_btnClose);

    connect(m_btnSave, &QPushButton::clicked, this, &DetectionResultDialog::onSaveClicked);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);

    mainLayout->addLayout(bottomLayout);

    setStyleSheet(
        "QDialog { "
        "  background-color: #252526; "
        "}"
    );
}

void DetectionResultDialog::setupClassificationUI()
{
    // 使用在 setupUI 中已创建的 m_classificationPanel
    QVBoxLayout *layout = new QVBoxLayout(m_classificationPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // Top-1 预测显示
    m_lblTopPrediction = new QLabel(m_classificationPanel);
    m_lblTopPrediction->setStyleSheet(
        "QLabel { "
        "  background-color: #1e6b3d; "
        "  color: #ffffff; "
        "  padding: 12px; "
        "  border-radius: 6px; "
        "  font-size: 16px; "
        "  font-weight: bold; "
        "}"
    );
    m_lblTopPrediction->setAlignment(Qt::AlignCenter);
    m_lblTopPrediction->setWordWrap(true);
    layout->addWidget(m_lblTopPrediction);

    // Top-K 表格
    m_classificationTable = new QTableWidget(m_classificationPanel);
    m_classificationTable->setColumnCount(3);
    m_classificationTable->setHorizontalHeaderLabels({tr("排名"), tr("类别"), tr("置信度")});
    m_classificationTable->horizontalHeader()->setStretchLastSection(true);
    m_classificationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_classificationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_classificationTable->setStyleSheet(
        "QTableWidget { "
        "  background-color: #1e1e1e; "
        "  color: #d4d4d4; "
        "  gridline-color: #3d3d3d; "
        "  border: 1px solid #3d3d3d; "
        "  border-radius: 4px; "
        "} "
        "QTableWidget::item { "
        "  padding: 6px; "
        "} "
        "QTableWidget::item:selected { "
        "  background-color: #094771; "
        "} "
        "QHeaderView::section { "
        "  background-color: #2d2d2d; "
        "  color: #ffffff; "
        "  padding: 6px; "
        "  border: none; "
        "  border-bottom: 1px solid #3d3d3d; "
        "}"
    );
    layout->addWidget(m_classificationTable);

    // 设置最小宽度
    m_classificationPanel->setMinimumWidth(250);
}

void DetectionResultDialog::setupComparisonUI()
{
    // 图像对比面板
    m_comparisonPanel = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(m_comparisonPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_comparisonSplitter = new QSplitter(Qt::Horizontal, this);

    // 原图视图
    QWidget *originalWidget = new QWidget(this);
    QVBoxLayout *originalLayout = new QVBoxLayout(originalWidget);
    originalLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *lblOriginal = new QLabel(tr("原始图像"), this);
    lblOriginal->setAlignment(Qt::AlignCenter);
    lblOriginal->setStyleSheet("QLabel { color: #888; font-size: 12px; }");
    m_originalView = new ImageView(this);
    m_originalView->setStyleSheet("ImageView { border: 2px solid #3d3d3d; border-radius: 4px; }");
    originalLayout->addWidget(lblOriginal);
    originalLayout->addWidget(m_originalView);

    // 处理后视图
    QWidget *processedWidget = new QWidget(this);
    QVBoxLayout *processedLayout = new QVBoxLayout(processedWidget);
    processedLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *lblProcessed = new QLabel(tr("处理后图像"), this);
    lblProcessed->setAlignment(Qt::AlignCenter);
    lblProcessed->setStyleSheet("QLabel { color: #888; font-size: 12px; }");
    m_processedView = new ImageView(this);
    m_processedView->setStyleSheet("ImageView { border: 2px solid #3d3d3d; border-radius: 4px; }");
    processedLayout->addWidget(lblProcessed);
    processedLayout->addWidget(m_processedView);

    m_comparisonSplitter->addWidget(originalWidget);
    m_comparisonSplitter->addWidget(processedWidget);
    m_comparisonSplitter->setSizes({500, 500});

    layout->addWidget(m_comparisonSplitter);
    m_comparisonPanel->setVisible(false);
}

void DetectionResultDialog::setResult(const QString &imagePath, const Utils::YOLODetectionResult &result, bool showLabels)
{
    m_currentImagePath = imagePath;
    m_currentResult = result;
    m_currentTaskType = Models::CVTask::ObjectDetection;

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        m_lblInfo->setText(tr("无法加载图像: %1").arg(imagePath));
        return;
    }

    setResult(pixmap, result, showLabels);
}

void DetectionResultDialog::setResult(const QPixmap &pixmap, const Utils::YOLODetectionResult &result, bool showLabels)
{
    m_originalPixmap = pixmap;
    m_currentResult = result;
    m_currentTaskType = Models::CVTask::ObjectDetection;

    // 显示主视图，隐藏其他面板
    m_imageView->setVisible(true);
    m_classificationPanel->setVisible(false);
    m_comparisonPanel->setVisible(false);
    m_txtDetails->setVisible(true);

    // 清除之前的检测结果
    m_imageView->clearDetections();
    m_imageView->clearImage();

    // 设置图像
    m_imageView->setPixmap(pixmap);

    // 转换并设置检测结果
    QVector<DetectionOverlay> overlays;
    for (int i = 0; i < result.detections.size(); ++i) {
        overlays.append(convertToOverlay(result.detections[i], i));
    }

    if (!overlays.isEmpty()) {
        m_imageView->setDetections(overlays, showLabels);
    }

    setWindowTitle(tr("检测结果"));
    updateInfoPanel();
}

void DetectionResultDialog::setSegmentationResult(const QPixmap &pixmap, const Utils::YOLODetectionResult &result,
                                                   int maskAlpha, bool showBoxes, bool showLabels)
{
    m_originalPixmap = pixmap;
    m_currentResult = result;
    m_currentTaskType = Models::CVTask::SemanticSegmentation;

    // 显示主视图，隐藏其他面板
    m_imageView->setVisible(true);
    m_classificationPanel->setVisible(false);
    m_comparisonPanel->setVisible(false);
    m_txtDetails->setVisible(true);

    // 清除之前的检测结果
    m_imageView->clearDetections();
    m_imageView->clearImage();

    // 设置图像
    m_imageView->setPixmap(pixmap);

    // 转换并设置分割结果（使用蒙版显示）
    QVector<DetectionOverlay> overlays;
    for (int i = 0; i < result.detections.size(); ++i) {
        overlays.append(convertToOverlay(result.detections[i], i));
    }

    if (!overlays.isEmpty()) {
        m_imageView->setSegmentationOverlays(overlays, maskAlpha, showBoxes, showLabels);
    }

    setWindowTitle(tr("语义分割结果"));
    updateInfoPanel();
}

void DetectionResultDialog::setClassificationResult(const QPixmap &pixmap, const Utils::YOLOClassificationResult &result)
{
    Q_UNUSED(pixmap)  // 分类结果不需要显示图片
    m_classificationResult = result;
    m_currentTaskType = Models::CVTask::ImageClassification;

    // 只显示分类面板，隐藏图像视图
    m_imageView->setVisible(false);
    m_classificationPanel->setVisible(true);
    m_comparisonPanel->setVisible(false);
    m_txtDetails->setVisible(false);

    // 让分类面板占据全部空间
    m_contentSplitter->setSizes({0, 1});

    setWindowTitle(tr("分类结果"));
    updateClassificationPanel();
}

void DetectionResultDialog::setKeypointResult(const QPixmap &pixmap, const Utils::YOLOKeypointResult &result,
                                               bool showBoxes, bool showLabels)
{
    m_originalPixmap = pixmap;
    m_keypointResult = result;
    m_currentTaskType = Models::CVTask::KeyPointDetection;

    // 显示主视图
    m_imageView->setVisible(true);
    m_classificationPanel->setVisible(false);
    m_comparisonPanel->setVisible(false);
    m_txtDetails->setVisible(true);

    // 清除之前的检测结果
    m_imageView->clearDetections();
    m_imageView->clearImage();

    // 绘制带骨架的图像
    QPixmap resultPixmap = pixmap;
    QPainter painter(&resultPixmap);

    // 预定义的颜色
    static const QVector<QColor> colors = {
        QColor(255, 87, 87), QColor(87, 255, 87), QColor(87, 87, 255),
        QColor(255, 255, 87), QColor(255, 87, 255), QColor(87, 255, 255),
    };

    for (int i = 0; i < result.detections.size(); ++i) {
        const auto &det = result.detections[i];
        QColor color = colors[i % colors.size()];

        // 绘制边界框（如果需要）
        if (showBoxes) {
            QPen pen(color, 3);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(det.x, det.y, det.width, det.height);
        }

        // 绘制骨架
        drawKeypointSkeleton(painter, det, color);

        // 绘制标签（如果需要）
        if (showLabels) {
            QString labelText = QString("%1 (%2%)")
                .arg(det.label)
                .arg(static_cast<int>(det.confidence * 100));

            QFont labelFont("Arial", 11, QFont::Bold);
            painter.setFont(labelFont);
            QFontMetrics fm(labelFont);
            QRect textRect = fm.boundingRect(labelText);
            textRect.moveTo(det.x, det.y - textRect.height() - 4);
            if (textRect.top() < 0) textRect.moveTo(det.x, det.y + 4);

            painter.fillRect(textRect.adjusted(-4, -2, 4, 2), color);
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter, labelText);
        }
    }

    painter.end();

    m_imageView->setPixmap(resultPixmap);

    // 更新信息
    QString infoText = tr("检测到 %1 个目标 | 耗时: %2ms")
        .arg(result.detections.size())
        .arg(static_cast<int>(result.inferenceTime));
    m_lblInfo->setText(infoText);

    // 详细信息
    QString details;
    details = tr("关键点检测结果:\n");
    details += QString("-").repeated(50) + "\n";
    for (int i = 0; i < result.detections.size(); ++i) {
        const auto &det = result.detections[i];
        details += tr("[%1] %2 - %3个关键点 | 置信度: %4%\n")
            .arg(i + 1)
            .arg(det.label)
            .arg(det.keypoints.size())
            .arg(static_cast<int>(det.confidence * 100));
    }
    m_txtDetails->setText(details);
    m_txtDetails->setVisible(true);

    setWindowTitle(tr("关键点检测结果"));
}

void DetectionResultDialog::setImageProcessResult(const QPixmap &originalPixmap, const QPixmap &processedPixmap,
                                                   const QString &processType, double processTime)
{
    m_originalPixmap = originalPixmap;
    m_processedPixmap = processedPixmap;
    m_processType = processType;

    // 只显示处理后的结果图像
    m_imageView->setVisible(true);
    m_classificationPanel->setVisible(false);
    m_comparisonPanel->setVisible(false);
    m_txtDetails->setVisible(false);

    // 清除并设置处理后的图像
    m_imageView->clearDetections();
    m_imageView->clearImage();
    m_imageView->setPixmap(processedPixmap);

    // 更新信息
    QString infoText = tr("%1 完成 | 耗时: %2ms | 图像尺寸: %3x%4")
        .arg(processType)
        .arg(static_cast<int>(processTime))
        .arg(processedPixmap.width())
        .arg(processedPixmap.height());
    m_lblInfo->setText(infoText);

    setWindowTitle(tr("%1 结果").arg(processType));
}

void DetectionResultDialog::drawKeypointSkeleton(QPainter &painter, const Utils::YOLOKeypointDetection &kp, const QColor &color)
{
    if (kp.keypoints.isEmpty()) return;

    // 绘制关键点
    painter.setPen(QPen(color, 2));
    painter.setBrush(QBrush(color));

    for (const auto &point : kp.keypoints) {
        if (point.x > 0 && point.y > 0) {
            painter.drawEllipse(QPointF(point.x, point.y), 4, 4);
        }
    }

    // COCO 骨架连接 (17点)
    // 0: nose, 1: left_eye, 2: right_eye, 3: left_ear, 4: right_ear
    // 5: left_shoulder, 6: right_shoulder, 7: left_elbow, 8: right_elbow
    // 9: left_wrist, 10: right_wrist, 11: left_hip, 12: right_hip
    // 13: left_knee, 14: right_knee, 15: left_ankle, 16: right_ankle

    if (kp.keypoints.size() >= 17) {
        QPen skeletonPen(QColor(color.red(), color.green(), color.blue(), 180), 2);
        painter.setPen(skeletonPen);
        painter.setBrush(Qt::NoBrush);

        // 头部连接
        QVector<QPair<int, int>> connections = {
            {0, 1}, {0, 2}, {1, 3}, {2, 4},  // 头部
            {5, 6},  // 肩膀
            {5, 7}, {7, 9},  // 左臂
            {6, 8}, {8, 10}, // 右臂
            {5, 11}, {6, 12}, // 躯干
            {11, 13}, {13, 15}, // 左腿
            {12, 14}, {14, 16}, // 右腿
            {11, 12}  // 臀部
        };

        for (const auto &conn : connections) {
            const auto &p1 = kp.keypoints[conn.first];
            const auto &p2 = kp.keypoints[conn.second];
            if (p1.x > 0 && p1.y > 0 && p2.x > 0 && p2.y > 0) {
                painter.drawLine(QPointF(p1.x, p1.y), QPointF(p2.x, p2.y));
            }
        }
    }
}

void DetectionResultDialog::clearResult()
{
    m_imageView->clearDetections();
    m_imageView->clearImage();
    m_originalView->clearImage();
    m_processedView->clearImage();
    m_originalPixmap = QPixmap();
    m_processedPixmap = QPixmap();
    m_currentResult = Utils::YOLODetectionResult();
    m_classificationResult = Utils::YOLOClassificationResult();
    m_keypointResult = Utils::YOLOKeypointResult();
    m_currentImagePath.clear();
    m_lblInfo->setText(tr("暂无结果"));
    m_txtDetails->setVisible(false);
    m_classificationPanel->setVisible(false);
    m_comparisonPanel->setVisible(false);
    m_imageView->setVisible(true);
}

DetectionOverlay DetectionResultDialog::convertToOverlay(const Utils::YOLODetection &det, int index)
{
    DetectionOverlay overlay;

    static const QVector<QColor> colors = {
        QColor(255, 87, 87), QColor(87, 255, 87), QColor(87, 87, 255),
        QColor(255, 255, 87), QColor(255, 87, 255), QColor(87, 255, 255),
        QColor(255, 165, 0), QColor(148, 87, 255), QColor(255, 192, 203),
        QColor(0, 255, 127),
    };

    overlay.x = det.x;
    overlay.y = det.y;
    overlay.width = det.width;
    overlay.height = det.height;
    overlay.confidence = det.confidence;
    overlay.label = det.label.isEmpty() ? QString("Class %1").arg(det.classId) : det.label;
    overlay.color = colors[index % colors.size()];

    // 复制掩码多边形数据
    for (const auto &pt : det.maskPolygon) {
        overlay.maskPolygon.append(QPointF(pt.x, pt.y));
    }

    return overlay;
}

void DetectionResultDialog::updateInfoPanel()
{
    if (!m_currentResult.success) {
        m_lblInfo->setText(tr("处理失败: %1").arg(m_currentResult.message));
        m_txtDetails->setVisible(false);
        return;
    }

    QString infoText;
    if (!m_currentImagePath.isEmpty()) {
        QFileInfo fileInfo(m_currentImagePath);
        infoText = tr("文件: %1 | ").arg(fileInfo.fileName());
    }
    infoText += tr("检测到 %1 个目标 | 耗时: %2ms")
        .arg(m_currentResult.detections.size())
        .arg(static_cast<int>(m_currentResult.inferenceTime));

    m_lblInfo->setText(infoText);

    if (!m_currentResult.detections.isEmpty()) {
        QString details;
        details = tr("检测结果详情:\n");
        details += QString("-").repeated(60) + "\n";

        QMap<QString, int> classCounts;
        for (const auto &det : m_currentResult.detections) {
            QString label = det.label.isEmpty() ? QString("Class %1").arg(det.classId) : det.label;
            classCounts[label]++;
        }

        details += tr("类别统计:\n");
        for (auto it = classCounts.constBegin(); it != classCounts.constEnd(); ++it) {
            details += tr("  %1: %2 个\n").arg(it.key()).arg(it.value());
        }

        details += QString("-").repeated(60) + "\n";
        details += tr("详细列表:\n");

        for (int i = 0; i < m_currentResult.detections.size(); ++i) {
            const auto &det = m_currentResult.detections[i];
            QString label = det.label.isEmpty() ? QString("Class %1").arg(det.classId) : det.label;
            details += tr("  [%1] %2 - 置信度: %3% | 位置: (%4, %5) %6x%7\n")
                .arg(i + 1)
                .arg(label)
                .arg(static_cast<int>(det.confidence * 100))
                .arg(det.x)
                .arg(det.y)
                .arg(det.width)
                .arg(det.height);
        }

        m_txtDetails->setText(details);
        m_txtDetails->setVisible(true);
    } else {
        m_txtDetails->setVisible(false);
    }
}

void DetectionResultDialog::updateClassificationPanel()
{
    if (!m_classificationResult.success) {
        m_lblTopPrediction->setText(tr("分类失败: %1").arg(m_classificationResult.message));
        m_classificationTable->setRowCount(0);
        return;
    }

    // 更新 Top-1 预测
    if (!m_classificationResult.classifications.isEmpty()) {
        const auto &top = m_classificationResult.topPrediction;
        m_lblTopPrediction->setText(
            tr("预测结果: %1\n置信度: %2%")
                .arg(top.label)
                .arg(static_cast<int>(top.confidence * 100))
        );
    } else {
        m_lblTopPrediction->setText(tr("无分类结果"));
    }

    // 更新信息标签
    QString infoText = tr("分类完成 | 耗时: %1ms | Top-%2 结果")
        .arg(static_cast<int>(m_classificationResult.inferenceTime))
        .arg(m_classificationResult.classifications.size());
    m_lblInfo->setText(infoText);

    // 更新表格
    m_classificationTable->setRowCount(m_classificationResult.classifications.size());
    for (int i = 0; i < m_classificationResult.classifications.size(); ++i) {
        const auto &cls = m_classificationResult.classifications[i];

        QTableWidgetItem *rankItem = new QTableWidgetItem(QString("#%1").arg(cls.rank));
        rankItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *labelItem = new QTableWidgetItem(cls.label);

        QTableWidgetItem *confItem = new QTableWidgetItem(
            QString("%1%").arg(static_cast<int>(cls.confidence * 100))
        );
        confItem->setTextAlignment(Qt::AlignCenter);

        // 第一行高亮
        if (i == 0) {
            QColor highlightColor(30, 107, 61);
            rankItem->setBackground(highlightColor);
            labelItem->setBackground(highlightColor);
            confItem->setBackground(highlightColor);
            rankItem->setForeground(Qt::white);
            labelItem->setForeground(Qt::white);
            confItem->setForeground(Qt::white);
        }

        m_classificationTable->setItem(i, 0, rankItem);
        m_classificationTable->setItem(i, 1, labelItem);
        m_classificationTable->setItem(i, 2, confItem);
    }

    m_classificationTable->resizeColumnsToContents();
}

void DetectionResultDialog::onSaveClicked()
{
    if (m_originalPixmap.isNull() && m_processedPixmap.isNull()) {
        QMessageBox::warning(this, tr("保存失败"), tr("没有可保存的图像"));
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(
        this,
        tr("保存结果图像"),
        QString(),
        tr("PNG图像 (*.png);;JPEG图像 (*.jpg *.jpeg);;BMP图像 (*.bmp);;所有文件 (*)")
    );

    if (savePath.isEmpty()) {
        return;
    }

    QPixmap pixmapToSave;

    // 根据任务类型选择保存的图像
    // 如果有处理后的图像（图像处理任务），优先保存处理后的图像
    if (!m_processedPixmap.isNull()) {
        pixmapToSave = m_processedPixmap;
    } else {
        pixmapToSave = m_originalPixmap;
    }

    // 如果是检测结果，绘制标注
    if (m_currentTaskType == Models::CVTask::ObjectDetection ||
        m_currentTaskType == Models::CVTask::SemanticSegmentation) {
        QImage resultImage(m_originalPixmap.size(), QImage::Format_ARGB32);
        resultImage.fill(Qt::transparent);

        QPainter painter(&resultImage);
        painter.drawPixmap(0, 0, m_originalPixmap);

        QFont labelFont("Arial", 12, QFont::Bold);
        painter.setFont(labelFont);

        for (int i = 0; i < m_currentResult.detections.size(); ++i) {
            const auto &det = m_currentResult.detections[i];
            DetectionOverlay overlay = convertToOverlay(det, i);

            QPen pen(overlay.color, 3);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(det.x, det.y, det.width, det.height);

            QString labelText = QString("%1 (%2%)")
                .arg(overlay.label)
                .arg(static_cast<int>(overlay.confidence * 100));

            QFontMetrics fm(labelFont);
            QRect textRect = fm.boundingRect(labelText);
            textRect.moveTo(det.x, det.y - textRect.height() - 4);
            if (textRect.top() < 0) textRect.moveTo(det.x, det.y + 4);

            painter.fillRect(textRect.adjusted(-4, -2, 4, 2), overlay.color);
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter, labelText);
        }

        painter.end();
        pixmapToSave = QPixmap::fromImage(resultImage);
    }

    if (pixmapToSave.save(savePath)) {
        QMessageBox::information(this, tr("保存成功"), tr("结果图像已保存至: %1").arg(savePath));
    } else {
        QMessageBox::warning(this, tr("保存失败"), tr("无法保存图像到指定位置"));
    }
}

void DetectionResultDialog::onZoomIn()
{
    if (m_imageView->isVisible()) {
        m_imageView->scaleImage(1.25);
    } else if (m_comparisonPanel->isVisible()) {
        m_originalView->scaleImage(1.25);
        m_processedView->scaleImage(1.25);
    }
}

void DetectionResultDialog::onZoomOut()
{
    if (m_imageView->isVisible()) {
        m_imageView->scaleImage(0.8);
    } else if (m_comparisonPanel->isVisible()) {
        m_originalView->scaleImage(0.8);
        m_processedView->scaleImage(0.8);
    }
}

void DetectionResultDialog::onFitToWindow()
{
    if (m_imageView->isVisible()) {
        m_imageView->fitToWindow();
    } else if (m_comparisonPanel->isVisible()) {
        m_originalView->fitToWindow();
        m_processedView->fitToWindow();
    }
}

void DetectionResultDialog::onActualSize()
{
    if (m_imageView->isVisible()) {
        m_imageView->actualSize();
    } else if (m_comparisonPanel->isVisible()) {
        m_originalView->actualSize();
        m_processedView->actualSize();
    }
}

} // namespace Views
} // namespace GenPreCVSystem
