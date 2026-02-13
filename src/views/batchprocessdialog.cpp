/**
 * @file batchprocessdialog.cpp
 * @brief 批量处理对话框实现
 *
 * 支持对文件夹中的图像进行批量 YOLO 推理处理：
 * - 支持图像分类、目标检测、语义分割、姿态检测
 * - 可配置置信度、IOU 阈值、图像尺寸
 * - 支持递归扫描子目录
 * - 导出为 ZIP 格式（包含 images 和 labels 文件夹）
 * - 生成 YOLO 格式标注文件
 */

#include "batchprocessdialog.h"
#include "../utils/appsettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QProgressBar>
#include <QTimer>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QPair>
#include <QPolygonF>
#include <algorithm>

using namespace GenPreCVSystem::Utils;

namespace GenPreCVSystem {
namespace Views {

BatchProcessDialog::BatchProcessDialog(QWidget *parent)
    : QDialog(parent)
    , m_yoloService(nullptr)
    , m_taskType(Models::CVTask::ImageClassification)
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
    setWindowTitle(tr("⚡ 批量处理"));
    setMinimumSize(550, 520);
    resize(600, 560);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // ========== 1. 任务和模型设置组 ==========
    QGroupBox *taskGroup = new QGroupBox(tr("🎯 任务和模型"), this);
    QFormLayout *taskLayout = new QFormLayout(taskGroup);

    // 任务类型选择（顺序：分类→检测→分割→Pose）
    m_comboTaskType = new QComboBox();
    m_comboTaskType->addItem(tr("图像分类"), static_cast<int>(Models::CVTask::ImageClassification));
    m_comboTaskType->addItem(tr("目标检测"), static_cast<int>(Models::CVTask::ObjectDetection));
    m_comboTaskType->addItem(tr("语义分割"), static_cast<int>(Models::CVTask::SemanticSegmentation));
    m_comboTaskType->addItem(tr("姿态检测"), static_cast<int>(Models::CVTask::KeyPointDetection));
    taskLayout->addRow(tr("任务类型:"), m_comboTaskType);

    // 模型选择
    QWidget *modelWidget = new QWidget();
    QHBoxLayout *modelLayout = new QHBoxLayout(modelWidget);
    modelLayout->setContentsMargins(0, 0, 0, 0);
    m_comboModel = new QComboBox();
    m_comboModel->setMinimumWidth(200);
    m_btnBrowseModel = new QPushButton(tr("📂 浏览..."));
    m_btnBrowseModel->setFixedWidth(80);
    modelLayout->addWidget(m_comboModel, 1);
    modelLayout->addWidget(m_btnBrowseModel);
    taskLayout->addRow(tr("模型:"), modelWidget);

    // 模型状态
    m_lblModelStatus = new QLabel(tr("○ 模型状态: 未加载"));
    m_lblModelStatus->setStyleSheet("color: #666666;");
    taskLayout->addRow("", m_lblModelStatus);

    mainLayout->addWidget(taskGroup);

    // ========== 2. 检测参数组 ==========
    QGroupBox *paramsGroup = new QGroupBox(tr("⚙ 检测参数"), this);
    QHBoxLayout *paramsLayout = new QHBoxLayout(paramsGroup);

    paramsLayout->addWidget(new QLabel(tr("置信度:")));
    m_spinConfThreshold = new QDoubleSpinBox();
    m_spinConfThreshold->setRange(0.01, 1.0);
    m_spinConfThreshold->setSingleStep(0.05);
    m_spinConfThreshold->setDecimals(2);
    m_spinConfThreshold->setValue(AppSettings::defaultConfThreshold());
    paramsLayout->addWidget(m_spinConfThreshold);

    paramsLayout->addSpacing(15);

    paramsLayout->addWidget(new QLabel(tr("IOU:")));
    m_spinIOUThreshold = new QDoubleSpinBox();
    m_spinIOUThreshold->setRange(0.01, 1.0);
    m_spinIOUThreshold->setSingleStep(0.05);
    m_spinIOUThreshold->setDecimals(2);
    m_spinIOUThreshold->setValue(AppSettings::defaultIOUThreshold());
    paramsLayout->addWidget(m_spinIOUThreshold);

    paramsLayout->addSpacing(15);

    paramsLayout->addWidget(new QLabel(tr("尺寸:")));
    m_spinImageSize = new QSpinBox();
    m_spinImageSize->setRange(128, 2048);
    m_spinImageSize->setSingleStep(64);
    m_spinImageSize->setValue(AppSettings::defaultImageSize());
    paramsLayout->addWidget(m_spinImageSize);

    paramsLayout->addStretch();

    mainLayout->addWidget(paramsGroup);

    // ========== 3. 输入设置组 ==========
    QGroupBox *inputGroup = new QGroupBox(tr("📂 输入设置"), this);
    QFormLayout *inputLayout = new QFormLayout(inputGroup);

    // 文件夹选择
    QWidget *folderWidget = new QWidget();
    QHBoxLayout *folderLayout = new QHBoxLayout(folderWidget);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    m_editFolder = new QLineEdit();
    m_editFolder->setReadOnly(true);
    m_editFolder->setPlaceholderText(tr("选择要处理的文件夹..."));
    m_btnBrowse = new QPushButton(tr("📂 浏览..."));
    m_btnBrowse->setFixedWidth(80);
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

    // ========== 4. 进度区域 ==========
    QGroupBox *progressGroup = new QGroupBox(tr("📊 处理进度"), this);
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
    mainLayout->addStretch();

    // ========== 按钮区域 ==========
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_btnStart = new QPushButton(tr("▶ 开始处理"), this);
    m_btnStop = new QPushButton(tr("⏹ 停止"), this);
    m_btnExport = new QPushButton(tr("📦 导出 ZIP"), this);
    m_btnClose = new QPushButton(tr("✕ 关闭"), this);

    m_btnStop->setEnabled(false);
    m_btnExport->setEnabled(false);

    m_btnStart->setMinimumWidth(100);
    m_btnStop->setMinimumWidth(70);
    m_btnExport->setMinimumWidth(90);
    m_btnClose->setMinimumWidth(70);

    buttonLayout->addWidget(m_btnStart);
    buttonLayout->addWidget(m_btnStop);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_btnExport);
    buttonLayout->addWidget(m_btnClose);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(m_btnBrowse, &QPushButton::clicked, this, &BatchProcessDialog::onBrowseFolder);
    connect(m_btnBrowseModel, &QPushButton::clicked, this, &BatchProcessDialog::onBrowseModel);
    connect(m_comboTaskType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatchProcessDialog::onTaskTypeChanged);
    connect(m_comboModel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatchProcessDialog::onModelSelectionChanged);
    connect(m_btnStart, &QPushButton::clicked, this, &BatchProcessDialog::onStartProcessing);
    connect(m_btnStop, &QPushButton::clicked, this, &BatchProcessDialog::onStopProcessing);
    connect(m_btnExport, &QPushButton::clicked, this, &BatchProcessDialog::onExportResults);
    connect(m_btnClose, &QPushButton::clicked, this, &BatchProcessDialog::onClose);
}

void BatchProcessDialog::applyStyles()
{
    setStyleSheet(
        "QDialog { background-color: #ffffff; color: #000000; }"
        "QGroupBox { border: 1px solid #0066cc; border-radius: 0px; margin-top: 8px; padding-top: 8px; color: #000000; font-weight: bold; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #0066cc; }"
        "QLabel { color: #000000; }"
        "QLineEdit { background-color: #ffffff; color: #000000; border: 1px solid #c0c0c0; padding: 5px; border-radius: 0px; }"
        "QLineEdit:focus { border: 1px solid #0066cc; }"
        "QSpinBox, QDoubleSpinBox { background-color: #ffffff; color: #000000; border: 1px solid #c0c0c0; padding: 3px; border-radius: 0px; }"
        "QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid #0066cc; }"
        "QComboBox { background-color: #ffffff; color: #000000; border: 1px solid #c0c0c0; padding: 5px; border-radius: 0px; }"
        "QComboBox:focus { border: 1px solid #0066cc; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #ffffff; color: #000000; selection-background-color: #0066cc; selection-color: #ffffff; }"
        "QCheckBox { color: #000000; }"
        "QProgressBar { border: 1px solid #0066cc; border-radius: 0px; text-align: center; background-color: #e0e0e0; color: #000000; }"
        "QProgressBar::chunk { background-color: #0066cc; border-radius: 0px; }"
        "QPushButton { background-color: #0066cc; color: #ffffff; border: none; padding: 8px 16px; border-radius: 0px; min-width: 70px; }"
        "QPushButton:hover { background-color: #0077dd; }"
        "QPushButton:pressed { background-color: #0055aa; }"
        "QPushButton:disabled { background-color: #c0c0c0; color: #666666; }"
    );
}

void BatchProcessDialog::setYOLOService(Utils::YOLOService *service)
{
    m_yoloService = service;
    updateModelList();
    // 选择第一个模型（不自动加载，等用户点击开始时加载）
    if (m_comboModel->count() > 0 && !m_comboModel->itemData(0).isNull()) {
        m_comboModel->setCurrentIndex(0);
        m_currentModelPath = m_comboModel->itemData(0).toString();
        m_lblModelStatus->setText(tr("✓ 已选择: %1").arg(QFileInfo(m_currentModelPath).fileName()));
        m_lblModelStatus->setStyleSheet("color: #0066cc;");
    }
}

QString BatchProcessDialog::getModelDirectory() const
{
    // 根据任务类型确定子目录名
    QString subDir;
    switch (m_taskType) {
    case Models::CVTask::ImageClassification:
        subDir = "classification";
        break;
    case Models::CVTask::ObjectDetection:
        subDir = "detection";
        break;
    case Models::CVTask::SemanticSegmentation:
        subDir = "segmentation";
        break;
    case Models::CVTask::KeyPointDetection:
        subDir = "keypoint";
        break;
    default:
        subDir = "detection";
        break;
    }

    QString appDir = QCoreApplication::applicationDirPath();

    // 优先使用构建目录下的 models
    QString buildModelPath = appDir + "/models/" + subDir;
    if (QDir(buildModelPath).exists()) {
        return buildModelPath;
    }

    // 开发环境：尝试源代码目录
    // 向上查找项目根目录
    QDir dir(appDir);
    if (dir.cdUp() && dir.cd("src/resources/models/" + subDir)) {
        return dir.absolutePath();
    }

    // 返回默认路径（即使不存在，后续会创建提示）
    return buildModelPath;
}

void BatchProcessDialog::updateModelList()
{
    m_comboModel->clear();

    QString modelDir = getModelDirectory();
    QDir dir(modelDir);

    // 如果目录不存在，尝试创建
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QStringList filters;
    filters << "*.pt" << "*.pth" << "*.onnx" << "*.torchscript";

    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);
    for (const QFileInfo &fileInfo : fileList) {
        m_comboModel->addItem(fileInfo.fileName(), fileInfo.absoluteFilePath());
    }

    // 如果没有找到模型，添加提示
    if (m_comboModel->count() == 0) {
        m_comboModel->addItem(tr("请选择模型文件..."), QString());
        m_lblModelStatus->setText(tr("⚠ 未找到模型 (目录: %1)").arg(modelDir));
        m_lblModelStatus->setStyleSheet("color: #cc3300;");
    }
}

void BatchProcessDialog::tryAutoLoadFirstModel()
{
    // 此函数保留但不再自动加载
    // 模型会在用户点击"开始处理"时加载
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

void BatchProcessDialog::onBrowseModel()
{
    QString modelPath = QFileDialog::getOpenFileName(this, tr("选择模型文件"),
                                                      getModelDirectory(),
                                                      tr("模型文件 (*.pt *.pth *.onnx *.torchscript);;所有文件 (*.*)"));
    if (!modelPath.isEmpty()) {
        // 检查是否已在列表中
        int index = m_comboModel->findData(modelPath);
        if (index < 0) {
            m_comboModel->addItem(QFileInfo(modelPath).fileName(), modelPath);
            m_comboModel->setCurrentIndex(m_comboModel->count() - 1);
        } else {
            m_comboModel->setCurrentIndex(index);
        }
    }
}

void BatchProcessDialog::onTaskTypeChanged(int index)
{
    m_taskType = static_cast<Models::CVTask>(m_comboTaskType->itemData(index).toInt());
    updateModelList();
    // 选择第一个模型
    if (m_comboModel->count() > 0 && !m_comboModel->itemData(0).isNull()) {
        m_comboModel->setCurrentIndex(0);
        m_currentModelPath = m_comboModel->itemData(0).toString();
        m_lblModelStatus->setText(tr("✓ 已选择: %1").arg(QFileInfo(m_currentModelPath).fileName()));
        m_lblModelStatus->setStyleSheet("color: #0066cc;");
    }
}

void BatchProcessDialog::onModelSelectionChanged(int index)
{
    QString modelPath = m_comboModel->itemData(index).toString();
    if (!modelPath.isEmpty()) {
        m_currentModelPath = modelPath;
        m_lblModelStatus->setText(tr("✓ 已选择: %1").arg(QFileInfo(modelPath).fileName()));
        m_lblModelStatus->setStyleSheet("color: #0066cc;");
    }
}

void BatchProcessDialog::populateImageList(const QString &folderPath)
{
    m_imageFiles.clear();

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
    m_lblStatus->setText(tr("找到 %1 个图像文件").arg(m_imageFiles.size()));
}

void BatchProcessDialog::onStartProcessing()
{
    if (m_imageFiles.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("没有可处理的图像文件"));
        return;
    }

    if (!m_yoloService) {
        QMessageBox::warning(this, tr("提示"), tr("YOLO 服务未初始化"));
        return;
    }

    if (!m_yoloService->isRunning()) {
        QMessageBox::warning(this, tr("提示"), tr("YOLO 服务未运行，请先启动服务"));
        return;
    }

    // 检查是否选择了模型
    if (m_currentModelPath.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请先选择模型"));
        return;
    }

    // 检查并加载模型（如果未加载或加载了不同的模型）
    bool needLoadModel = !m_yoloService->isModelLoaded() ||
                         m_yoloService->modelPath() != m_currentModelPath;

    if (needLoadModel) {
        m_lblStatus->setText(tr("正在加载模型..."));
        QCoreApplication::processEvents();

        if (!m_yoloService->loadModel(m_currentModelPath)) {
            QMessageBox::warning(this, tr("提示"), tr("模型加载失败"));
            m_lblModelStatus->setText(tr("✗ 模型状态: 加载失败"));
            m_lblModelStatus->setStyleSheet("color: #cc3300;");
            return;
        }
        m_lblModelStatus->setText(tr("● 已加载: %1").arg(QFileInfo(m_currentModelPath).fileName()));
        m_lblModelStatus->setStyleSheet("color: #0066cc; font-weight: bold;");
    }

    // 重置状态
    m_currentIndex = 0;
    m_stopRequested = false;
    m_isProcessing = true;
    m_successCount = 0;
    m_failCount = 0;
    m_totalTime = 0.0;
    m_detectionResults.clear();
    m_classificationResults.clear();
    m_keypointResults.clear();

    // 更新 UI
    m_btnStart->setEnabled(false);
    m_btnStop->setEnabled(true);
    m_btnExport->setEnabled(false);
    m_btnBrowse->setEnabled(false);
    m_comboTaskType->setEnabled(false);
    m_comboModel->setEnabled(false);
    m_btnBrowseModel->setEnabled(false);
    m_progressBar->setValue(0);

    m_lblStatus->setText(tr("正在处理..."));

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

    m_lblStatus->setText(tr("处理: %1").arg(fileInfo.fileName()));
    updateProgress();

    // 根据任务类型执行不同的推理
    switch (m_taskType) {
    case Models::CVTask::ObjectDetection:
        {
            Utils::YOLODetectionResult result = m_yoloService->detect(imagePath,
                static_cast<float>(m_spinConfThreshold->value()),
                static_cast<float>(m_spinIOUThreshold->value()),
                m_spinImageSize->value());
            m_detectionResults.append({imagePath, result});
            if (result.success) {
                m_successCount++;
                m_totalTime += result.inferenceTime;
            } else {
                m_failCount++;
            }
        }
        break;

    case Models::CVTask::SemanticSegmentation:
        {
            Utils::YOLODetectionResult result = m_yoloService->segment(imagePath,
                static_cast<float>(m_spinConfThreshold->value()),
                static_cast<float>(m_spinIOUThreshold->value()),
                m_spinImageSize->value());
            m_detectionResults.append({imagePath, result});
            if (result.success) {
                m_successCount++;
                m_totalTime += result.inferenceTime;
            } else {
                m_failCount++;
            }
        }
        break;

    case Models::CVTask::ImageClassification:
        {
            Utils::YOLOClassificationResult result = m_yoloService->classify(imagePath);
            m_classificationResults.append({imagePath, result});
            if (result.success) {
                m_successCount++;
                m_totalTime += result.inferenceTime;
            } else {
                m_failCount++;
            }
        }
        break;

    case Models::CVTask::KeyPointDetection:
        {
            Utils::YOLOKeypointResult result = m_yoloService->keypoint(imagePath,
                static_cast<float>(m_spinConfThreshold->value()),
                static_cast<float>(m_spinIOUThreshold->value()),
                m_spinImageSize->value());
            m_keypointResults.append({imagePath, result});
            if (result.success) {
                m_successCount++;
                m_totalTime += result.inferenceTime;
            } else {
                m_failCount++;
            }
        }
        break;

    default:
        m_failCount++;
        break;
    }

    m_currentIndex++;

    // 使用单次定时器处理下一个（避免阻塞 UI）
    QTimer::singleShot(1, this, &BatchProcessDialog::processNextImage);
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
    m_comboTaskType->setEnabled(true);
    m_comboModel->setEnabled(true);
    m_btnBrowseModel->setEnabled(true);
    m_progressBar->setValue(100);
    m_lblProgress->setText(QString("%1 / %1").arg(m_imageFiles.size()));

    if (m_stopRequested) {
        m_lblStatus->setText(tr("已停止: 成功 %1, 失败 %2").arg(m_successCount).arg(m_failCount));
    } else {
        m_lblStatus->setText(tr("完成: 成功 %1, 失败 %2").arg(m_successCount).arg(m_failCount));
    }

    // 启用导出按钮
    if (m_successCount > 0) {
        m_btnExport->setEnabled(true);
    }
}

void BatchProcessDialog::onExportResults()
{
    if (m_successCount == 0) {
        QMessageBox::warning(this, tr("提示"), tr("没有可导出的结果"));
        return;
    }

    QString defaultDir = AppSettings::defaultExportDirectory();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString defaultPath = QString("%1/dataset_%2.zip").arg(defaultDir).arg(timestamp);

    QString zipPath = QFileDialog::getSaveFileName(this, tr("导出 ZIP"),
                                                    defaultPath,
                                                    tr("ZIP 压缩包 (*.zip)"));

    if (zipPath.isEmpty()) {
        return;
    }

    m_lblStatus->setText(tr("正在导出..."));
    QCoreApplication::processEvents();

    if (exportAsZip(zipPath)) {
        m_lblStatus->setText(tr("已导出: %1").arg(QFileInfo(zipPath).fileName()));
        QMessageBox::information(this, tr("导出成功"),
            tr("结果已导出至:\n%1\n\n包含 images/ 和 labels/ 文件夹").arg(zipPath));
        emit batchCompleted(zipPath);
    } else {
        QMessageBox::warning(this, tr("导出失败"), tr("无法创建 ZIP 文件"));
    }
}

bool BatchProcessDialog::exportAsZip(const QString &zipPath)
{
    // 创建临时目录
    QString tempDirPath = QDir::tempPath() + "/batch_export_" +
        QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir tempDir(tempDirPath);
    tempDir.mkpath("images");
    tempDir.mkpath("labels");

    QString imagesDir = tempDirPath + "/images";
    QString labelsDir = tempDirPath + "/labels";

    // 预定义颜色列表
    QList<QColor> colors = {
        QColor(255, 0, 0), QColor(0, 255, 0), QColor(0, 0, 255),
        QColor(255, 255, 0), QColor(255, 0, 255), QColor(0, 255, 255),
        QColor(255, 128, 0), QColor(128, 0, 255), QColor(0, 128, 255),
        QColor(255, 0, 128)
    };

    // COCO 骨架连接关系 (17关键点)
    // 0-nose, 1-left_eye, 2-right_eye, 3-left_ear, 4-right_ear,
    // 5-left_shoulder, 6-right_shoulder, 7-left_elbow, 8-right_elbow,
    // 9-left_wrist, 10-right_wrist, 11-left_hip, 12-right_hip,
    // 13-left_knee, 14-right_knee, 15-left_ankle, 16-right_ankle
    QList<QPair<int, int>> skeletonConnections = {
        {0, 1}, {0, 2}, {1, 3}, {2, 4},           // 头部
        {0, 5}, {0, 6},                           // 头到肩膀
        {5, 7}, {7, 9},                           // 左臂
        {6, 8}, {8, 10},                          // 右臂
        {5, 11}, {6, 12},                         // 肩膀到臀部
        {11, 13}, {13, 15},                       // 左腿
        {12, 14}, {14, 16}                        // 右腿
    };

    // 根据任务类型处理结果
    switch (m_taskType) {
    case Models::CVTask::ObjectDetection:
        // 目标检测 - 只绘制边界框
        for (const auto &resultPair : m_detectionResults) {
            const QString &imagePath = resultPair.first;
            const Utils::YOLODetectionResult &result = resultPair.second;

            if (!result.success) continue;

            QFileInfo imageInfo(imagePath);
            QString baseName = imageInfo.completeBaseName();

            QImage image(imagePath);
            if (image.isNull()) continue;

            QImage renderImage = image.convertToFormat(QImage::Format_ARGB32);
            QPainter painter(&renderImage);
            painter.setRenderHint(QPainter::Antialiasing);

            double imgWidth = image.width();
            double imgHeight = image.height();
            int fontSize = qMax(12, static_cast<int>(std::min(imgWidth, imgHeight) / 40.0));
            QFont font("Arial", fontSize, QFont::Bold);
            painter.setFont(font);

            QString labelContent;
            for (int i = 0; i < result.detections.size(); ++i) {
                const auto &det = result.detections[i];
                QColor color = colors[i % colors.size()];

                // 绘制边界框
                painter.setPen(QPen(color, 3));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(static_cast<int>(det.x), static_cast<int>(det.y),
                                 static_cast<int>(det.width), static_cast<int>(det.height));

                // 绘制标签: "className: 95.2%"
                QString detLabel = det.label.isEmpty()
                    ? QString("class_%1").arg(det.classId)
                    : det.label;
                QString label = QString("%1: %2%").arg(detLabel).arg(det.confidence * 100, 0, 'f', 1);
                QFontMetrics fm(font);
                QRect textRect = fm.boundingRect(label);
                textRect.moveTo(static_cast<int>(det.x), static_cast<int>(det.y) - textRect.height() - 4);
                textRect.setWidth(textRect.width() + 8);
                textRect.setHeight(textRect.height() + 4);

                painter.fillRect(textRect, color);
                painter.setPen(Qt::white);
                painter.drawText(textRect.adjusted(4, 2, -4, -2), Qt::AlignLeft | Qt::AlignVCenter, label);

                // 生成 YOLO 格式标签
                double centerX = (det.x + det.width / 2.0) / imgWidth;
                double centerY = (det.y + det.height / 2.0) / imgHeight;
                double normWidth = det.width / imgWidth;
                double normHeight = det.height / imgHeight;

                centerX = qBound(0.0, centerX, 1.0);
                centerY = qBound(0.0, centerY, 1.0);
                normWidth = qBound(0.0, normWidth, 1.0);
                normHeight = qBound(0.0, normHeight, 1.0);

                labelContent += QString("%1 %2 %3 %4 %5\n")
                    .arg(det.classId)
                    .arg(centerX, 0, 'f', 6)
                    .arg(centerY, 0, 'f', 6)
                    .arg(normWidth, 0, 'f', 6)
                    .arg(normHeight, 0, 'f', 6);
            }
            painter.end();

            QString destImagePath = QString("%1/%2.jpg").arg(imagesDir).arg(baseName);
            renderImage.save(destImagePath, "JPEG", 95);

            QFile labelFile(QString("%1/%2.txt").arg(labelsDir).arg(baseName));
            if (labelFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                labelFile.write(labelContent.toUtf8());
                labelFile.close();
            }
        }
        break;

    case Models::CVTask::SemanticSegmentation:
        // 语义分割 - 绘制蒙版和边界框
        for (const auto &resultPair : m_detectionResults) {
            const QString &imagePath = resultPair.first;
            const Utils::YOLODetectionResult &result = resultPair.second;

            if (!result.success) continue;

            QFileInfo imageInfo(imagePath);
            QString baseName = imageInfo.completeBaseName();

            QImage image(imagePath);
            if (image.isNull()) continue;

            QImage renderImage = image.convertToFormat(QImage::Format_ARGB32);
            QPainter painter(&renderImage);
            painter.setRenderHint(QPainter::Antialiasing);

            double imgWidth = image.width();
            double imgHeight = image.height();
            int fontSize = qMax(12, static_cast<int>(std::min(imgWidth, imgHeight) / 40.0));
            QFont font("Arial", fontSize, QFont::Bold);
            painter.setFont(font);

            QString labelContent;
            for (int i = 0; i < result.detections.size(); ++i) {
                const auto &det = result.detections[i];
                QColor color = colors[i % colors.size()];

                // 绘制蒙版（如果有）
                if (!det.maskPolygon.isEmpty() && det.maskPolygon.size() >= 3) {
                    QColor maskColor = color;
                    maskColor.setAlpha(100);  // 半透明
                    painter.setBrush(maskColor);
                    painter.setPen(Qt::NoPen);

                    QPolygonF polygon;
                    for (const auto &pt : det.maskPolygon) {
                        polygon << QPointF(pt.x, pt.y);
                    }
                    painter.drawPolygon(polygon);
                }

                // 绘制边界框
                painter.setPen(QPen(color, 2));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(static_cast<int>(det.x), static_cast<int>(det.y),
                                 static_cast<int>(det.width), static_cast<int>(det.height));

                // 绘制标签: "className: 95.2%"
                QString detLabel = det.label.isEmpty()
                    ? QString("class_%1").arg(det.classId)
                    : det.label;
                QString label = QString("%1: %2%").arg(detLabel).arg(det.confidence * 100, 0, 'f', 1);
                QFontMetrics fm(font);
                QRect textRect = fm.boundingRect(label);
                textRect.moveTo(static_cast<int>(det.x), static_cast<int>(det.y) - textRect.height() - 4);
                textRect.setWidth(textRect.width() + 8);
                textRect.setHeight(textRect.height() + 4);

                painter.fillRect(textRect, color);
                painter.setPen(Qt::white);
                painter.drawText(textRect.adjusted(4, 2, -4, -2), Qt::AlignLeft | Qt::AlignVCenter, label);

                // 生成 YOLO 格式标签
                double centerX = (det.x + det.width / 2.0) / imgWidth;
                double centerY = (det.y + det.height / 2.0) / imgHeight;
                double normWidth = det.width / imgWidth;
                double normHeight = det.height / imgHeight;

                centerX = qBound(0.0, centerX, 1.0);
                centerY = qBound(0.0, centerY, 1.0);
                normWidth = qBound(0.0, normWidth, 1.0);
                normHeight = qBound(0.0, normHeight, 1.0);

                labelContent += QString("%1 %2 %3 %4 %5\n")
                    .arg(det.classId)
                    .arg(centerX, 0, 'f', 6)
                    .arg(centerY, 0, 'f', 6)
                    .arg(normWidth, 0, 'f', 6)
                    .arg(normHeight, 0, 'f', 6);
            }
            painter.end();

            QString destImagePath = QString("%1/%2.jpg").arg(imagesDir).arg(baseName);
            renderImage.save(destImagePath, "JPEG", 95);

            QFile labelFile(QString("%1/%2.txt").arg(labelsDir).arg(baseName));
            if (labelFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                labelFile.write(labelContent.toUtf8());
                labelFile.close();
            }
        }
        break;

    case Models::CVTask::ImageClassification:
        // 图像分类 - 显示类别名称、ID和置信度
        {
            QString clsContent;
            for (const auto &resultPair : m_classificationResults) {
                const QString &imagePath = resultPair.first;
                const Utils::YOLOClassificationResult &result = resultPair.second;

                if (!result.success) continue;

                QFileInfo imageInfo(imagePath);
                QString baseName = imageInfo.completeBaseName();

                QImage image(imagePath);
                if (image.isNull()) continue;

                QImage renderImage = image.convertToFormat(QImage::Format_ARGB32);
                QPainter painter(&renderImage);

                if (!result.classifications.isEmpty()) {
                    const auto &topClass = result.classifications.first();
                    int fontSize = qMax(14, std::min(image.width(), image.height()) / 25);
                    QFont font("Arial", fontSize, QFont::Bold);
                    painter.setFont(font);
                    QFontMetrics fm(font);

                    // 处理空标签的情况
                    QString className = topClass.label.isEmpty()
                        ? QString("class_%1").arg(topClass.classId)
                        : topClass.label;

                    // 显示: "className (id): 95.2%"
                    QString label = QString("%1 (%2): %3%")
                        .arg(className)
                        .arg(topClass.classId)
                        .arg(topClass.confidence * 100, 0, 'f', 1);

                    QRect textRect = fm.boundingRect(label);
                    textRect.moveTo(10, 10);
                    textRect.setWidth(textRect.width() + 16);
                    textRect.setHeight(textRect.height() + 8);

                    painter.fillRect(textRect, QColor(0, 0, 0, 180));
                    painter.setPen(QColor(0, 255, 0));
                    painter.drawText(textRect.adjusted(8, 4, -8, -4), Qt::AlignLeft | Qt::AlignVCenter, label);

                    clsContent += QString("%1.jpg %2\n").arg(baseName).arg(topClass.classId);
                }
                painter.end();

                QString destImagePath = QString("%1/%2.jpg").arg(imagesDir).arg(baseName);
                renderImage.save(destImagePath, "JPEG", 95);
            }

            QFile clsFile(QString("%1/classes.txt").arg(labelsDir));
            if (clsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                clsFile.write(clsContent.toUtf8());
                clsFile.close();
            }
        }
        break;

    case Models::CVTask::KeyPointDetection:
        // 姿态检测 - 绘制骨架和关键点
        for (const auto &resultPair : m_keypointResults) {
            const QString &imagePath = resultPair.first;
            const Utils::YOLOKeypointResult &result = resultPair.second;

            if (!result.success) continue;

            QFileInfo imageInfo(imagePath);
            QString baseName = imageInfo.completeBaseName();

            QImage image(imagePath);
            if (image.isNull()) continue;

            QImage renderImage = image.convertToFormat(QImage::Format_ARGB32);
            QPainter painter(&renderImage);
            painter.setRenderHint(QPainter::Antialiasing);

            double imgWidth = image.width();
            double imgHeight = image.height();

            QString labelContent;
            for (int i = 0; i < result.detections.size(); ++i) {
                const auto &kp = result.detections[i];
                QColor color = colors[i % colors.size()];

                // 绘制边界框
                painter.setPen(QPen(color, 2));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(static_cast<int>(kp.x), static_cast<int>(kp.y),
                                 static_cast<int>(kp.width), static_cast<int>(kp.height));

                // 构建关键点ID映射
                QMap<int, QPointF> keypointMap;
                for (const auto &point : kp.keypoints) {
                    keypointMap[point.id] = QPointF(point.x, point.y);
                }

                // 绘制骨架连线
                painter.setPen(QPen(color, 2));
                for (const auto &conn : skeletonConnections) {
                    if (keypointMap.contains(conn.first) && keypointMap.contains(conn.second)) {
                        painter.drawLine(keypointMap[conn.first], keypointMap[conn.second]);
                    }
                }

                // 绘制关键点
                painter.setBrush(color);
                painter.setPen(QPen(Qt::white, 1));
                for (const auto &point : kp.keypoints) {
                    painter.drawEllipse(QPointF(point.x, point.y), 4, 4);
                }

                // 生成 YOLO pose 格式标签
                double centerX = (kp.x + kp.width / 2.0) / imgWidth;
                double centerY = (kp.y + kp.height / 2.0) / imgHeight;
                double normWidth = kp.width / imgWidth;
                double normHeight = kp.height / imgHeight;

                centerX = qBound(0.0, centerX, 1.0);
                centerY = qBound(0.0, centerY, 1.0);
                normWidth = qBound(0.0, normWidth, 1.0);
                normHeight = qBound(0.0, normHeight, 1.0);

                QString line = QString("%1 %2 %3 %4 %5")
                    .arg(kp.classId)
                    .arg(centerX, 0, 'f', 6)
                    .arg(centerY, 0, 'f', 6)
                    .arg(normWidth, 0, 'f', 6)
                    .arg(normHeight, 0, 'f', 6);

                for (const auto &point : kp.keypoints) {
                    double px = point.x / imgWidth;
                    double py = point.y / imgHeight;
                    line += QString(" %1 %2 %3")
                        .arg(px, 0, 'f', 6)
                        .arg(py, 0, 'f', 6)
                        .arg(point.confidence, 0, 'f', 2);
                }
                labelContent += line + "\n";
            }
            painter.end();

            QString destImagePath = QString("%1/%2.jpg").arg(imagesDir).arg(baseName);
            renderImage.save(destImagePath, "JPEG", 95);

            QFile labelFile(QString("%1/%2.txt").arg(labelsDir).arg(baseName));
            if (labelFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                labelFile.write(labelContent.toUtf8());
                labelFile.close();
            }
        }
        break;

    default:
        break;
    }

    // 使用 PowerShell 创建 ZIP 文件
    QString powershellCmd = QString(
        "Compress-Archive -Path '%1/images', '%1/labels' -DestinationPath '%2' -Force")
        .arg(tempDirPath)
        .arg(zipPath);

    QProcess process;
    process.start("powershell", QStringList() << "-Command" << powershellCmd);
    bool success = process.waitForFinished(60000);

    QDir(tempDirPath).removeRecursively();

    return success && process.exitCode() == 0;
}

void BatchProcessDialog::onStopProcessing()
{
    m_stopRequested = true;
    m_lblStatus->setText(tr("正在停止..."));
}

void BatchProcessDialog::onClose()
{
    if (m_isProcessing) {
        m_stopRequested = true;
        QCoreApplication::processEvents();
    }
    reject();
}

} // namespace Views
} // namespace GenPreCVSystem
