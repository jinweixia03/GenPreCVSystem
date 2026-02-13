#include "batchprocessdialog.h"
#include "../utils/appsettings.h"
#include "../utils/exportservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QHeaderView>
#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QProgressBar>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QCoreApplication>

using namespace GenPreCVSystem::Utils;

namespace GenPreCVSystem {
namespace Views {

BatchProcessDialog::BatchProcessDialog(QWidget *parent)
    : QDialog(parent)
    , m_yoloService(nullptr)
    , m_taskType(Models::CVTask::ObjectDetection)
    , m_currentIndex(0)
    , m_isProcessing(false)
    , m_stopRequested(false)
    , m_successCount(0)
    , m_failCount(0)
    , m_totalTime(0.0)
{
    setupUI();
    applyStyles();
}

BatchProcessDialog::~BatchProcessDialog()
{
}

void BatchProcessDialog::setupUI()
{
    setWindowTitle(tr("批量处理"));
    setMinimumSize(700, 600);
    resize(800, 650);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // ========== 输入设置组 ==========
    QGroupBox *inputGroup = new QGroupBox(tr("输入设置"), this);
    QFormLayout *inputLayout = new QFormLayout(inputGroup);

    // 文件夹选择
    QWidget *folderWidget = new QWidget();
    QHBoxLayout *folderLayout = new QHBoxLayout(folderWidget);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    m_editFolder = new QLineEdit();
    m_editFolder->setReadOnly(true);
    m_editFolder->setPlaceholderText(tr("选择要处理的文件夹..."));
    m_btnBrowse = new QPushButton(tr("浏览..."));
    folderLayout->addWidget(m_editFolder, 1);
    folderLayout->addWidget(m_btnBrowse);
    inputLayout->addRow(tr("文件夹:"), folderWidget);

    // 选项
    QWidget *optionsWidget = new QWidget();
    QHBoxLayout *optionsLayout = new QHBoxLayout(optionsWidget);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    m_chkRecursive = new QCheckBox(tr("包含子目录"));
    m_comboImageFormat = new QComboBox();
    m_comboImageFormat->addItem(tr("所有图片"), "*.jpg *.jpeg *.png *.bmp *.tif *.tiff *.webp");
    m_comboImageFormat->addItem("JPEG (*.jpg *.jpeg)", "*.jpg *.jpeg");
    m_comboImageFormat->addItem("PNG (*.png)", "*.png");
    m_comboImageFormat->addItem("BMP (*.bmp)", "*.bmp");
    optionsLayout->addWidget(m_chkRecursive);
    optionsLayout->addStretch();
    optionsLayout->addWidget(new QLabel(tr("格式:")));
    optionsLayout->addWidget(m_comboImageFormat);
    inputLayout->addRow("", optionsWidget);

    mainLayout->addWidget(inputGroup);

    // ========== 检测参数组 ==========
    QGroupBox *paramsGroup = new QGroupBox(tr("检测参数"), this);
    QHBoxLayout *paramsLayout = new QHBoxLayout(paramsGroup);

    paramsLayout->addWidget(new QLabel(tr("置信度阈值:")));
    m_spinConfThreshold = new QDoubleSpinBox();
    m_spinConfThreshold->setRange(0.01, 1.0);
    m_spinConfThreshold->setSingleStep(0.05);
    m_spinConfThreshold->setDecimals(2);
    m_spinConfThreshold->setValue(AppSettings::defaultConfThreshold());
    paramsLayout->addWidget(m_spinConfThreshold);

    paramsLayout->addSpacing(20);

    paramsLayout->addWidget(new QLabel(tr("IOU 阈值:")));
    m_spinIOUThreshold = new QDoubleSpinBox();
    m_spinIOUThreshold->setRange(0.01, 1.0);
    m_spinIOUThreshold->setSingleStep(0.05);
    m_spinIOUThreshold->setDecimals(2);
    m_spinIOUThreshold->setValue(AppSettings::defaultIOUThreshold());
    paramsLayout->addWidget(m_spinIOUThreshold);

    paramsLayout->addSpacing(20);

    paramsLayout->addWidget(new QLabel(tr("图像尺寸:")));
    m_spinImageSize = new QSpinBox();
    m_spinImageSize->setRange(128, 2048);
    m_spinImageSize->setSingleStep(64);
    m_spinImageSize->setValue(AppSettings::defaultImageSize());
    paramsLayout->addWidget(m_spinImageSize);

    paramsLayout->addStretch();

    mainLayout->addWidget(paramsGroup);

    // ========== 进度区域 ==========
    QGroupBox *progressGroup = new QGroupBox(tr("处理进度"), this);
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);

    // 进度条
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    progressLayout->addWidget(m_progressBar);

    // 状态标签
    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_lblStatus = new QLabel(tr("等待开始..."));
    m_lblProgress = new QLabel("0 / 0");
    m_lblProgress->setAlignment(Qt::AlignRight);
    statusLayout->addWidget(m_lblStatus, 1);
    statusLayout->addWidget(m_lblProgress);
    progressLayout->addLayout(statusLayout);

    mainLayout->addWidget(progressGroup);

    // ========== 结果表格 ==========
    QGroupBox *resultsGroup = new QGroupBox(tr("处理结果"), this);
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsGroup);

    m_resultsTable = new QTableWidget();
    m_resultsTable->setColumnCount(4);
    m_resultsTable->setHorizontalHeaderLabels({tr("文件名"), tr("检测数"), tr("耗时(ms)"), tr("状态")});
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTable->setAlternatingRowColors(true);
    resultsLayout->addWidget(m_resultsTable);

    mainLayout->addWidget(resultsGroup, 1);

    // ========== 日志区域 ==========
    QGroupBox *logGroup = new QGroupBox(tr("日志"), this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

    m_txtLog = new QTextEdit();
    m_txtLog->setReadOnly(true);
    m_txtLog->setMaximumHeight(100);
    logLayout->addWidget(m_txtLog);

    mainLayout->addWidget(logGroup);

    // ========== 按钮区域 ==========
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_btnStart = new QPushButton(tr("开始处理"), this);
    m_btnStop = new QPushButton(tr("停止"), this);
    m_btnExport = new QPushButton(tr("导出结果"), this);
    m_btnClose = new QPushButton(tr("关闭"), this);

    m_btnStop->setEnabled(false);
    m_btnExport->setEnabled(false);

    buttonLayout->addWidget(m_btnStart);
    buttonLayout->addWidget(m_btnStop);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_btnExport);
    buttonLayout->addWidget(m_btnClose);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(m_btnBrowse, &QPushButton::clicked, this, &BatchProcessDialog::onBrowseFolder);
    connect(m_btnStart, &QPushButton::clicked, this, &BatchProcessDialog::onStartProcessing);
    connect(m_btnStop, &QPushButton::clicked, this, &BatchProcessDialog::onStopProcessing);
    connect(m_btnExport, &QPushButton::clicked, this, &BatchProcessDialog::onExportResults);
    connect(m_btnClose, &QPushButton::clicked, this, &BatchProcessDialog::onClose);
}

void BatchProcessDialog::applyStyles()
{
    setStyleSheet(
        "QDialog { background-color: #1e1e1e; color: #cccccc; }"
        "QGroupBox { border: 1px solid #3e3e42; border-radius: 4px; margin-top: 8px; padding-top: 8px; color: #cccccc; font-weight: bold; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
        "QLabel { color: #cccccc; }"
        "QLineEdit { background-color: #3c3c3c; color: #cccccc; border: 1px solid #3e3e42; padding: 5px; border-radius: 2px; }"
        "QSpinBox, QDoubleSpinBox { background-color: #3c3c3c; color: #cccccc; border: 1px solid #3e3e42; padding: 3px; border-radius: 2px; }"
        "QComboBox { background-color: #3c3c3c; color: #cccccc; border: 1px solid #3e3e42; padding: 5px; border-radius: 2px; }"
        "QCheckBox { color: #cccccc; }"
        "QProgressBar { border: 1px solid #3e3e42; border-radius: 3px; text-align: center; background-color: #3c3c3c; }"
        "QProgressBar::chunk { background-color: #0078d4; border-radius: 2px; }"
        "QTableWidget { background-color: #252526; color: #cccccc; border: 1px solid #3e3e42; gridline-color: #3e3e42; }"
        "QTableWidget::item { padding: 5px; }"
        "QTableWidget::item:selected { background-color: #094771; }"
        "QHeaderView::section { background-color: #2d2d30; color: #cccccc; padding: 5px; border: 1px solid #3e3e42; }"
        "QTextEdit { background-color: #1e1e1e; color: #cccccc; border: 1px solid #3e3e42; font-family: Consolas, monospace; }"
        "QPushButton { background-color: #0e639c; color: #ffffff; border: none; padding: 8px 16px; border-radius: 2px; min-width: 80px; }"
        "QPushButton:hover { background-color: #1177bb; }"
        "QPushButton:pressed { background-color: #0e639c; }"
        "QPushButton:disabled { background-color: #3c3c3c; color: #666666; }"
    );
}

void BatchProcessDialog::setYOLOService(Utils::YOLOService *service)
{
    m_yoloService = service;
}

void BatchProcessDialog::setTaskType(Models::CVTask task)
{
    m_taskType = task;
}

void BatchProcessDialog::setModelPath(const QString &path)
{
    m_modelPath = path;
}

void BatchProcessDialog::onBrowseFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择要处理的文件夹"),
                                                     AppSettings::defaultOpenDirectory());
    if (!dir.isEmpty()) {
        m_editFolder->setText(dir);
        m_currentFolder = dir;
        populateImageList(dir);
    }
}

void BatchProcessDialog::populateImageList(const QString &folderPath)
{
    m_imageFiles.clear();
    m_resultsTable->setRowCount(0);

    QDir dir(folderPath);
    QString filter = m_comboImageFormat->currentData().toString();
    QStringList filters = filter.split(" ", Qt::SkipEmptyParts);

    if (m_chkRecursive->isChecked()) {
        QDirIterator it(folderPath, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            m_imageFiles.append(it.next());
        }
    } else {
        QStringList files = dir.entryList(filters, QDir::Files);
        for (const QString &file : files) {
            m_imageFiles.append(dir.filePath(file));
        }
    }

    m_lblProgress->setText(QString("0 / %1").arg(m_imageFiles.size()));
    addLogEntry(tr("找到 %1 个图像文件").arg(m_imageFiles.size()));
}

void BatchProcessDialog::onStartProcessing()
{
    if (m_imageFiles.isEmpty()) {
        addLogEntry(tr("没有可处理的图像文件"));
        return;
    }

    if (!m_yoloService || !m_yoloService->isRunning()) {
        addLogEntry(tr("YOLO 服务未运行，请先启动服务"));
        return;
    }

    if (!m_yoloService->isModelLoaded()) {
        addLogEntry(tr("模型未加载，请先加载模型"));
        return;
    }

    // 重置状态
    m_currentIndex = 0;
    m_stopRequested = false;
    m_isProcessing = true;
    m_successCount = 0;
    m_failCount = 0;
    m_totalTime = 0.0;
    m_results.clear();
    m_resultsTable->setRowCount(0);

    // 更新 UI
    m_btnStart->setEnabled(false);
    m_btnStop->setEnabled(true);
    m_btnExport->setEnabled(false);
    m_btnBrowse->setEnabled(false);
    m_progressBar->setValue(0);

    addLogEntry(tr("开始批量处理..."));
    addLogEntry(tr("任务类型: %1").arg(static_cast<int>(m_taskType)));

    // 开始处理
    processNextImage();
}

void BatchProcessDialog::processNextImage()
{
    if (m_stopRequested || m_currentIndex >= m_imageFiles.size()) {
        finishProcessing();
        return;
    }

    QString imagePath = m_imageFiles[m_currentIndex];
    QFileInfo fileInfo(imagePath);

    m_lblStatus->setText(tr("正在处理: %1").arg(fileInfo.fileName()));
    updateProgress();

    // 执行检测
    Utils::YOLODetectionResult result;
    switch (m_taskType) {
    case Models::CVTask::ObjectDetection:
        result = m_yoloService->detect(imagePath,
                                        static_cast<float>(m_spinConfThreshold->value()),
                                        static_cast<float>(m_spinIOUThreshold->value()),
                                        m_spinImageSize->value());
        break;
    case Models::CVTask::SemanticSegmentation:
        result = m_yoloService->segment(imagePath,
                                         static_cast<float>(m_spinConfThreshold->value()),
                                         static_cast<float>(m_spinIOUThreshold->value()),
                                         m_spinImageSize->value());
        break;
    default:
        result = m_yoloService->detect(imagePath,
                                        static_cast<float>(m_spinConfThreshold->value()),
                                        static_cast<float>(m_spinIOUThreshold->value()),
                                        m_spinImageSize->value());
        break;
    }

    // 记录结果
    m_results.append({imagePath, result});

    // 更新统计
    if (result.success) {
        m_successCount++;
        m_totalTime += result.inferenceTime;
        addResultRow(fileInfo.fileName(), result.detections.size(), result.inferenceTime, tr("成功"));
        addLogEntry(tr("完成: %1 (%2 个检测, %3ms)")
            .arg(fileInfo.fileName())
            .arg(result.detections.size())
            .arg(result.inferenceTime, 0, 'f', 1));
    } else {
        m_failCount++;
        addResultRow(fileInfo.fileName(), 0, 0, tr("失败"));
        addLogEntry(tr("失败: %1 - %2").arg(fileInfo.fileName()).arg(result.message));
    }

    m_currentIndex++;

    // 使用单次定时器处理下一个（避免阻塞 UI）
    QTimer::singleShot(10, this, &BatchProcessDialog::processNextImage);
}

void BatchProcessDialog::updateProgress()
{
    int total = m_imageFiles.size();
    if (total > 0) {
        int progress = static_cast<int>((m_currentIndex * 100.0) / total);
        m_progressBar->setValue(progress);
        m_lblProgress->setText(QString("%1 / %2").arg(m_currentIndex).arg(total));
    }
}

void BatchProcessDialog::finishProcessing()
{
    m_isProcessing = false;

    // 更新 UI
    m_btnStart->setEnabled(true);
    m_btnStop->setEnabled(false);
    m_btnBrowse->setEnabled(true);
    m_progressBar->setValue(100);

    QString statusText = tr("处理完成: 成功 %1, 失败 %2")
        .arg(m_successCount).arg(m_failCount);
    m_lblStatus->setText(statusText);
    m_lblProgress->setText(QString("%1 / %1").arg(m_imageFiles.size()));

    // 显示汇总
    addLogEntry("========================================");
    addLogEntry(tr("批量处理完成"));
    addLogEntry(tr("总图片数: %1").arg(m_imageFiles.size()));
    addLogEntry(tr("成功: %1, 失败: %2").arg(m_successCount).arg(m_failCount));
    addLogEntry(tr("总检测数: %1").arg(
        std::accumulate(m_results.begin(), m_results.end(), 0,
            [](int sum, const QPair<QString, Utils::YOLODetectionResult> &r) {
                return sum + r.second.detections.size();
            })));
    addLogEntry(tr("平均耗时: %1 ms").arg(
        m_successCount > 0 ? m_totalTime / m_successCount : 0, 0, 'f', 1));

    // 启用导出按钮
    if (!m_results.isEmpty()) {
        m_btnExport->setEnabled(true);
    }
}

void BatchProcessDialog::onStopProcessing()
{
    m_stopRequested = true;
    m_lblStatus->setText(tr("正在停止..."));
    addLogEntry(tr("用户请求停止处理"));
}

void BatchProcessDialog::onExportResults()
{
    if (m_results.isEmpty()) {
        return;
    }

    QString defaultDir = AppSettings::defaultExportDirectory();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString defaultName = QString("batch_results_%1.json").arg(timestamp);

    QString filePath = QFileDialog::getSaveFileName(this, tr("导出结果"),
                                                     defaultDir + "/" + defaultName,
                                                     Utils::ExportService::getExportFilter());

    if (filePath.isEmpty()) {
        return;
    }

    Utils::ExportMetadata metadata;
    metadata.modelName = m_modelPath;
    metadata.taskType = QString::number(static_cast<int>(m_taskType));
    metadata.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    Utils::ExportService::Format format = Utils::ExportService::formatFromExtension(
        QFileInfo(filePath).suffix());

    bool success = Utils::ExportService::exportBatchResults(m_results, metadata, filePath, format);

    if (success) {
        addLogEntry(tr("结果已导出至: %1").arg(filePath));
        emit batchCompleted(filePath);
    } else {
        addLogEntry(tr("导出失败"));
    }
}

void BatchProcessDialog::onClose()
{
    if (m_isProcessing) {
        m_stopRequested = true;
        // 等待处理停止
        QCoreApplication::processEvents();
    }
    reject();
}

void BatchProcessDialog::addLogEntry(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_txtLog->append(QString("[%1] %2").arg(timestamp).arg(message));
}

void BatchProcessDialog::addResultRow(const QString &fileName, int detectionCount, double time, const QString &status)
{
    int row = m_resultsTable->rowCount();
    m_resultsTable->insertRow(row);

    m_resultsTable->setItem(row, 0, new QTableWidgetItem(fileName));
    m_resultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(detectionCount)));
    m_resultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(time, 'f', 1)));
    m_resultsTable->setItem(row, 3, new QTableWidgetItem(status));

    // 滚动到最新行
    m_resultsTable->scrollToBottom();
}

} // namespace Views
} // namespace GenPreCVSystem
