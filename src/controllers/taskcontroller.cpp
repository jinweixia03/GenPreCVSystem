/**
 * @file taskcontroller.cpp
 * @brief 任务控制器实现
 *
 * 管理计算机视觉任务的切换和执行：
 * - 图像分类、目标检测、语义分割、姿态检测
 * - 图像增强、去噪、边缘检测
 *
 * 协调 DL 服务、图像处理服务和 UI 组件
 */

#include "taskcontroller.h"
#include "tasktypes.h"
#include "parameterpanelfactory.h"
#include "tabcontroller.h"
#include "dlservice.h"
#include "imageprocessservice.h"
#include "detectionresultdialog.h"
#include "environmentservicewidget.h"
#include "mainwindow.h"  // 包含 ImageView 定义
#include <QScrollArea>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QPixmap>
#include <QSlider>
#include <QTimer>
#include <QPointer>
#include <QDebug>
#include <QCoreApplication>
#include <QFileInfoList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDialog>
#include <QGroupBox>

namespace GenPreCVSystem {
namespace Controllers {

TaskController::TaskController(QObject *parent)
    : QObject(parent)
    , m_paramScrollArea(nullptr)
    , m_taskActionGroup(nullptr)
    , m_currentTask(Models::CVTask::ImageClassification)
    , m_dlService(nullptr)
    , m_imageProcessService(nullptr)
    , m_tabController(nullptr)
    , m_tabWidget(nullptr)
    , m_resultDialog(nullptr)
    , m_envServiceWidget(nullptr)
    , m_taskParamContainer(nullptr)
    , m_maskAlpha(50)
    , m_showBoxes(false)
    , m_showLabels(true)
{
    // 创建 DL 服务
    m_dlService = new Utils::DLService(this);

    // 创建图像处理服务
    m_imageProcessService = new Utils::ImageProcessService(this);

    // 创建检测结果对话框
    m_resultDialog = new Views::DetectionResultDialog(nullptr);

    // 创建共享的环境服务控件
    m_envServiceWidget = new Widgets::EnvironmentServiceWidget(nullptr);
    m_envServiceWidget->setDLService(m_dlService);

    // 连接 DL 服务信号
    connect(m_dlService, &Utils::DLService::logMessage,
            this, &TaskController::logMessage);
    connect(m_dlService, &Utils::DLService::detectionCompleted,
            this, &TaskController::onDetectionCompleted);

    // 连接图像处理服务信号
    connect(m_imageProcessService, &Utils::ImageProcessService::logMessage,
            this, &TaskController::logMessage);

    // 连接共享控件信号
    connectSharedWidgetSignals();
}

TaskController::~TaskController()
{
    // DL 服务会被 Qt 的父子关系自动删除
    if (m_dlService) {
        m_dlService->stop();
    }
    // 删除结果对话框
    if (m_resultDialog) {
        delete m_resultDialog;
        m_resultDialog = nullptr;
    }
    // 删除共享控件
    if (m_envServiceWidget) {
        delete m_envServiceWidget;
        m_envServiceWidget = nullptr;
    }
}

void TaskController::connectSharedWidgetSignals()
{
    // 转发日志消息
    connect(m_envServiceWidget, &Widgets::EnvironmentServiceWidget::logMessage,
            this, &TaskController::logMessage);

    // 模型加载成功后，启用执行按钮
    connect(m_envServiceWidget, &Widgets::EnvironmentServiceWidget::modelLoaded,
            this, [this](const QString &modelPath) {
        m_currentModelPath = modelPath;
        enableRunButtons(true);
    });

    // 服务停止后，禁用执行按钮
    connect(m_envServiceWidget, &Widgets::EnvironmentServiceWidget::serviceStopped,
            this, [this]() {
        enableRunButtons(false);
    });
}

void TaskController::enableRunButtons(bool enabled)
{
    if (!m_taskParamContainer) return;

    // 查找所有执行按钮
    QStringList runButtonNames = {
        "btnRunDetection",
        "btnRunSegmentation",
        "btnRunClassification",
        "btnRunKeyPoint",
        "btnRunRoadDamage",
        "btnRunManholeCover",
        "btnRunFewShotClassification"
    };

    for (const QString &btnName : runButtonNames) {
        QPushButton *runBtn = m_taskParamContainer->findChild<QPushButton *>(btnName);
        if (runBtn) {
            runBtn->setEnabled(enabled);
            if (enabled) {
                runBtn->setStyleSheet("QPushButton:enabled { background-color: #2196F3; color: white; font-weight: bold; padding: 6px; }");
            } else {
                runBtn->setStyleSheet("QPushButton:disabled { background-color: #cccccc; color: #666666; }");
            }
        }
    }
}

bool TaskController::isAITask(Models::CVTask task) const
{
    switch (task) {
        case Models::CVTask::ImageClassification:
        case Models::CVTask::ObjectDetection:
        case Models::CVTask::SemanticSegmentation:
        case Models::CVTask::KeyPointDetection:
        case Models::CVTask::RoadDamageDetection:
        case Models::CVTask::ManholeCoverDamageDetection:
        case Models::CVTask::RemoteSceneFewShotClassification:
            return true;
        default:
            return false;
    }
}

QString TaskController::getModelDirectory(Models::CVTask task)
{
    QString appDir = QCoreApplication::applicationDirPath();

    // 根据任务类型确定子目录名
    QString subDir;
    switch (task) {
        case Models::CVTask::ObjectDetection:
            subDir = "detection";
            break;
        case Models::CVTask::SemanticSegmentation:
            subDir = "segmentation";
            break;
        case Models::CVTask::ImageClassification:
            subDir = "classification";
            break;
        case Models::CVTask::KeyPointDetection:
            subDir = "keypoint";
            break;
        case Models::CVTask::RoadDamageDetection:
            subDir = "roaddamagedetection";
            break;
        case Models::CVTask::ManholeCoverDamageDetection:
            subDir = "manholecoverdamagedetection";
            break;
        case Models::CVTask::RemoteSceneFewShotClassification:
            subDir = "remotescenefewshotclassification";
            break;
        default:
            subDir = "detection";
            break;
    }

    // 优先使用构建目录下的 models（CMake 复制目标）
    QString buildModelPath = appDir + "/models/" + subDir;
    if (QDir(buildModelPath).exists()) {
        return buildModelPath;
    }

    // 开发环境：尝试源代码目录
    QDir dir(appDir);
    QStringList possiblePaths = {
        appDir + "/../src/resources/models/" + subDir,
        appDir + "/src/resources/models/" + subDir,
        appDir + "/../../src/resources/models/" + subDir,
    };

    // 尝试从构建目录找到源目录
    if (dir.cdUp()) {
        possiblePaths.prepend(dir.absolutePath() + "/src/resources/models/" + subDir);
    }

    for (const QString &path : possiblePaths) {
        QString normalized = QDir::cleanPath(path);
        if (QDir(normalized).exists()) {
            return normalized;
        }
    }

    // 返回默认路径（即使不存在，后续会提示）
    return buildModelPath;
}

QStringList TaskController::scanAvailableModels(Models::CVTask task)
{
    QStringList models;
    QString modelDir = getModelDirectory(task);

    QDir dir(modelDir);
    if (!dir.exists()) {
        qDebug() << "Model directory does not exist:" << modelDir;
        return models;
    }

    // 支持的模型文件扩展名
    QStringList filters;
    filters << "*.pt" << "*.pth" << "*.onnx" << "*.torchscript";

    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable);

    for (const QFileInfo &fileInfo : fileList) {
        models.append(fileInfo.absoluteFilePath());
    }

    qDebug() << "Found" << models.size() << "models in" << modelDir;
    return models;
}

void TaskController::setParameterScrollArea(QScrollArea *scrollArea)
{
    m_paramScrollArea = scrollArea;
}

void TaskController::setTaskActionGroup(QActionGroup *actionGroup)
{
    m_taskActionGroup = actionGroup;
}

void TaskController::setTabController(TabController *tabController)
{
    m_tabController = tabController;
}

void TaskController::setTabWidget(QTabWidget *tabWidget)
{
    m_tabWidget = tabWidget;
}

// 辅助方法：获取当前 ImageView
::ImageView* TaskController::getCurrentImageView() const
{
    // 使用 QTabWidget 获取当前的 ImageView
    if (m_tabWidget) {
        QWidget *currentWidget = m_tabWidget->currentWidget();
        if (currentWidget) {
            // 使用全局命名空间的 ImageView（定义在 mainwindow.h 中）
            return qobject_cast<::ImageView*>(currentWidget);
        }
    }
    return nullptr;
}

QString TaskController::getCurrentImagePath() const
{
    // 优先使用 TabController
    if (m_tabController) {
        QString path = m_tabController->currentImagePath();
        if (!path.isEmpty()) {
            return path;
        }
    }
    // 回退到直接设置的路径
    return m_currentImagePath;
}

QString TaskController::getCurrentImageForInference()
{
    // 清理之前的临时文件
    cleanupTempImage();

    // 获取当前显示的图片（直接从 ImageView 获取，确保是处理后的图片）
    QPixmap currentPixmap;
    ::ImageView *imageView = getCurrentImageView();
    if (imageView) {
        currentPixmap = imageView->pixmap();
    }

    // 如果无法从 ImageView 获取，尝试从 TabController 获取
    if (currentPixmap.isNull() && m_tabController) {
        currentPixmap = m_tabController->currentPixmap();
    }

    // 缓存当前 pixmap 用于结果显示
    m_currentPixmap = currentPixmap;

    // 如果无法获取当前图片，回退到原始文件路径
    if (currentPixmap.isNull()) {
        // 尝试从文件路径加载并缓存
        QString imagePath = getCurrentImagePath();
        if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
            m_currentPixmap.load(imagePath);
        }
        return imagePath;
    }

    // 使用应用程序目录创建临时文件（避免中文路径问题）
    QString appDir = QCoreApplication::applicationDirPath();
    QString tempDirPath = appDir + "/temp";
    QDir tempDir(tempDirPath);
    if (!tempDir.exists()) {
        tempDir.mkpath(".");
    }

    // 生成唯一的临时文件名（使用时间戳和随机数，确保只包含 ASCII 字符）
    QString tempFileName = QString("inference_%1_%2.png")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QRandomGenerator::global()->bounded(10000));
    QString tempPath = tempDirPath + "/" + tempFileName;

    // 保存图片到临时文件
    if (!currentPixmap.save(tempPath, "PNG")) {
        emit logMessage("无法保存临时图像");
        return getCurrentImagePath();
    }

    m_tempImagePath = tempPath;

    emit logMessage(QString("使用当前显示的图像进行推理"));
    return tempPath;
}

void TaskController::cleanupTempImage()
{
    if (!m_tempImagePath.isEmpty() && QFile::exists(m_tempImagePath)) {
        QFile::remove(m_tempImagePath);
        m_tempImagePath.clear();
    }
}

void TaskController::setCurrentImagePath(const QString &imagePath)
{
    m_currentImagePath = imagePath;
}

void TaskController::setCurrentPixmap(const QPixmap &pixmap)
{
    m_currentPixmap = pixmap;
}

void TaskController::switchTask(Models::CVTask task)
{
    m_currentTask = task;

    // 检查是否需要切换服务类型
    bool needRestart = false;
    QString newTaskType;

    if (task == Models::CVTask::RemoteSceneFewShotClassification) {
        newTaskType = "fsl";
        emit logMessage("[任务] FSL 任务需要使用包含 EasyFSL 的 Python 环境（如 fsl 环境）");
        // 提示用户选择正确的环境
        if (m_envServiceWidget) {
            // 检查当前环境是否包含 easyfsl
            QString currentEnv = m_dlService ? m_dlService->currentEnvironmentPath() : QString();
            emit logMessage(QString("[任务] 当前环境: %1").arg(currentEnv.isEmpty() ? "未选择" : currentEnv));
            if (!currentEnv.contains("fsl")) {
                emit logMessage("⚠️ [任务] 警告: 请在环境面板中选择 'fsl' conda 环境");
            }
        }
    } else if (isAITask(task)) {
        newTaskType = "dl";
    }

    // 如果任务类型变化且服务正在运行，需要重启服务
    if (m_dlService && !newTaskType.isEmpty() && m_dlService->taskType() != newTaskType) {
        if (m_dlService->isRunning()) {
            emit logMessage("任务类型变化，需要重启服务...");
            m_dlService->stop();
            needRestart = true;
        }
        m_dlService->setTaskType(newTaskType);
        emit logMessage(QString("服务类型已切换为: %1").arg(newTaskType));
    }

    updateParameterPanel(task);

    // 如果服务被停止且需要重启，通过环境服务控件触发自动启动
    if (needRestart && m_envServiceWidget) {
        emit logMessage("正在自动重启服务...");
        QTimer::singleShot(300, this, [this]() {
            if (m_envServiceWidget) {
                m_envServiceWidget->tryAutoStartService();
            }
        });
    }

    emit taskChanged(task);
    emit logMessage(QString("已切换任务: %1").arg(Models::getTaskName(task)));
}

void TaskController::updateParameterPanel(Models::CVTask task)
{
    clearParameterPanel();

    // 创建主容器
    QWidget *container = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    // 对于 AI 任务，添加共享的环境服务控件
    if (isAITask(task)) {
        // 共享控件可能已经有父控件，需要先移除
        if (m_envServiceWidget->parent() != nullptr) {
            m_envServiceWidget->setParent(nullptr);
        }
        containerLayout->addWidget(m_envServiceWidget);

        // 更新模型列表（如果服务已运行，内部会自动尝试加载新模型）
        m_envServiceWidget->updateModelList(task);

        // 更新服务状态显示
        m_envServiceWidget->updateServiceStatus();
    }

    // 创建任务特定的参数面板
    QWidget *taskPanel = ParameterPanelFactory::createParameterPanel(task);
    if (taskPanel) {
        containerLayout->addWidget(taskPanel);
        m_taskParamContainer = taskPanel;

        // 连接任务面板的信号
        connectParameterPanelSignals();

        // 如果模型已加载，启用执行按钮
        if (isAITask(task) && m_dlService && m_dlService->isModelLoaded()) {
            enableRunButtons(true);
        }
    }

    containerLayout->addStretch();

    if (m_paramScrollArea) {
        m_paramScrollArea->setWidget(container);
        m_paramScrollArea->setWidgetResizable(true);
    }
}

void TaskController::clearParameterPanel()
{
    m_taskParamContainer = nullptr;

    if (m_paramScrollArea && m_paramScrollArea->widget()) {
        // 从容器中移除共享控件（防止被删除）
        if (m_envServiceWidget) {
            m_envServiceWidget->setParent(nullptr);
        }

        delete m_paramScrollArea->widget();
        m_paramScrollArea->setWidget(nullptr);
    }
}

bool TaskController::startDLService()
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return false;
    }

    if (m_dlService->isRunning()) {
        emit logMessage("服务已在运行中");
        return true;
    }

    emit logMessage("正在启动服务...");
    bool success = m_dlService->start();

    if (success) {
        emit logMessage("服务启动成功");
    } else {
        emit logMessage("服务启动失败");
    }

    return success;
}

void TaskController::stopDLService()
{
    if (m_dlService) {
        m_dlService->stop();
        emit logMessage("服务已停止");
    }
}

bool TaskController::loadDLModel(const QString &modelPath, const QString &labelsPath)
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return false;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("服务未运行，请先启动服务");
        return false;
    }

    emit logMessage(QString("正在加载模型: %1").arg(modelPath));
    bool success = m_dlService->loadModel(modelPath, labelsPath);

    return success;
}

void TaskController::runDetection(const QString &imagePath, float confThreshold,
                                   float iouThreshold, int imageSize)
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("服务未运行，请先启动服务");
        return;
    }

    // 保存当前图像路径，用于显示结果
    m_currentImagePath = imagePath;

    emit logMessage(QString("执行目标检测: %1").arg(imagePath));

    // 同步调用检测（实际应用中可以考虑异步）
    Utils::DetectionResult result = m_dlService->detect(
        imagePath, confThreshold, iouThreshold, imageSize);

    // 注意：信号 detectionCompleted 会在 detect() 内部发射，
    // 触发 onDetectionCompleted() 显示结果对话框
}

void TaskController::runSegmentation(const QString &imagePath, float confThreshold,
                                      float iouThreshold, int imageSize)
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("服务未运行，请先启动服务");
        return;
    }

    // 保存当前图像路径，用于显示结果
    m_currentImagePath = imagePath;

    emit logMessage(QString("执行实例分割: %1").arg(imagePath));

    // 同步调用分割
    Utils::DetectionResult result = m_dlService->segment(
        imagePath, confThreshold, iouThreshold, imageSize);

    // 注意：信号 detectionCompleted 会在 segment() 内部发射，
    // 触发 onDetectionCompleted() 显示结果对话框
}

void TaskController::connectParameterPanelSignals()
{
    if (!m_taskParamContainer) {
        return;
    }

    QWidget *panel = m_taskParamContainer;

    // 查找执行检测按钮
    QPushButton *runDetectionBtn = panel->findChild<QPushButton *>("btnRunDetection");

    if (runDetectionBtn) {
        connect(runDetectionBtn, &QPushButton::clicked, this, [this, panel, runDetectionBtn]() {
            // 获取当前显示的图像用于推理（可能是处理后的图像）
            QString imagePath = getCurrentImageForInference();

            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            // 禁用按钮防止重复点击
            runDetectionBtn->setEnabled(false);
            runDetectionBtn->setText("检测中...");

            // 获取参数
            QDoubleSpinBox *confSpinBox = panel->findChild<QDoubleSpinBox *>("spinConfThreshold");
            QDoubleSpinBox *iouSpinBox = panel->findChild<QDoubleSpinBox *>("spinIOUThreshold");
            QSpinBox *sizeSpinBox = panel->findChild<QSpinBox *>("spinImageSize");
            QCheckBox *showLabelsCheck = panel->findChild<QCheckBox *>("chkShowLabels");

            float confThreshold = confSpinBox ? static_cast<float>(confSpinBox->value()) : 0.25f;
            float iouThreshold = iouSpinBox ? static_cast<float>(iouSpinBox->value()) : 0.45f;
            int imageSize = sizeSpinBox ? sizeSpinBox->value() : 640;

            // 保存显示设置
            m_showLabels = showLabelsCheck ? showLabelsCheck->isChecked() : true;

            runDetection(imagePath, confThreshold, iouThreshold, imageSize);

            // 清理临时文件
            cleanupTempImage();

            // 恢复按钮状态
            runDetectionBtn->setEnabled(true);
            runDetectionBtn->setText("执行检测");
        });
    }

    // 查找执行分割按钮
    QPushButton *runSegmentationBtn = panel->findChild<QPushButton *>("btnRunSegmentation");

    if (runSegmentationBtn) {
        connect(runSegmentationBtn, &QPushButton::clicked, this, [this, panel, runSegmentationBtn]() {
            // 获取当前显示的图像用于推理（可能是处理后的图像）
            QString imagePath = getCurrentImageForInference();

            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            // 禁用按钮防止重复点击
            runSegmentationBtn->setEnabled(false);
            runSegmentationBtn->setText("分割中...");

            // 获取参数
            QDoubleSpinBox *confSpinBox = panel->findChild<QDoubleSpinBox *>("spinConfThreshold");
            QDoubleSpinBox *iouSpinBox = panel->findChild<QDoubleSpinBox *>("spinIOUThreshold");
            QSpinBox *sizeSpinBox = panel->findChild<QSpinBox *>("spinImageSize");
            QSlider *maskAlphaSlider = panel->findChild<QSlider *>("sliderMaskAlpha");
            QCheckBox *showBoxesCheck = panel->findChild<QCheckBox *>("chkShowBoxes");
            QCheckBox *showLabelsCheck = panel->findChild<QCheckBox *>("chkShowLabels");

            float confThreshold = confSpinBox ? static_cast<float>(confSpinBox->value()) : 0.25f;
            float iouThreshold = iouSpinBox ? static_cast<float>(iouSpinBox->value()) : 0.45f;
            int imageSize = sizeSpinBox ? sizeSpinBox->value() : 640;

            // 保存分割显示设置
            m_maskAlpha = maskAlphaSlider ? maskAlphaSlider->value() : 50;
            m_showBoxes = showBoxesCheck ? showBoxesCheck->isChecked() : false;
            m_showLabels = showLabelsCheck ? showLabelsCheck->isChecked() : true;

            runSegmentation(imagePath, confThreshold, iouThreshold, imageSize);

            // 清理临时文件
            cleanupTempImage();

            // 恢复按钮状态
            runSegmentationBtn->setEnabled(true);
            runSegmentationBtn->setText("执行语义分割");
        });
    }

    // ========== 图像分类按钮 ==========
    QPushButton *runClassificationBtn = panel->findChild<QPushButton *>("btnRunClassification");
    if (runClassificationBtn) {
        connect(runClassificationBtn, &QPushButton::clicked, this, [this, panel, runClassificationBtn]() {
            // 获取当前显示的图像用于推理（可能是处理后的图像）
            QString imagePath = getCurrentImageForInference();

            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            // 禁用按钮防止重复点击
            runClassificationBtn->setEnabled(false);
            runClassificationBtn->setText("分类中...");

            // 获取参数
            QSpinBox *topKSpinBox = panel->findChild<QSpinBox *>("spinTopK");
            int topK = topKSpinBox ? topKSpinBox->value() : 5;

            runClassification(imagePath, topK);

            // 清理临时文件
            cleanupTempImage();

            // 恢复按钮状态
            runClassificationBtn->setEnabled(true);
            runClassificationBtn->setText("执行分类");
        });
    }

    // ========== 关键点检测按钮 ==========
    QPushButton *runKeyPointBtn = panel->findChild<QPushButton *>("btnRunKeyPoint");
    if (runKeyPointBtn) {
        connect(runKeyPointBtn, &QPushButton::clicked, this, [this, panel, runKeyPointBtn]() {
            // 获取当前显示的图像用于推理（可能是处理后的图像）
            QString imagePath = getCurrentImageForInference();

            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            // 禁用按钮防止重复点击
            runKeyPointBtn->setEnabled(false);
            runKeyPointBtn->setText("检测中...");

            // 获取参数
            QDoubleSpinBox *confSpinBox = panel->findChild<QDoubleSpinBox *>("spinConfThreshold");
            QSpinBox *sizeSpinBox = panel->findChild<QSpinBox *>("spinImageSize");
            QCheckBox *showBoxesCheck = panel->findChild<QCheckBox *>("chkShowBoxes");
            QCheckBox *showLabelsCheck = panel->findChild<QCheckBox *>("chkShowLabels");

            float confThreshold = confSpinBox ? static_cast<float>(confSpinBox->value()) : 0.25f;
            int imageSize = sizeSpinBox ? sizeSpinBox->value() : 640;

            // 保存显示设置
            m_showBoxes = showBoxesCheck ? showBoxesCheck->isChecked() : true;
            m_showLabels = showLabelsCheck ? showLabelsCheck->isChecked() : true;

            runKeypointDetection(imagePath, confThreshold, 0.45f, imageSize);

            // 清理临时文件
            cleanupTempImage();

            // 恢复按钮状态
            runKeyPointBtn->setEnabled(true);
            runKeyPointBtn->setText("执行关键点检测");
        });
    }

    // ========== 道路病害检测按钮 ==========
    QPushButton *runRoadDamageBtn = panel->findChild<QPushButton *>("btnRunRoadDamage");
    if (runRoadDamageBtn) {
        connect(runRoadDamageBtn, &QPushButton::clicked, this, [this, panel, runRoadDamageBtn]() {
            // 获取当前显示的图像用于推理（可能是处理后的图像）
            QString imagePath = getCurrentImageForInference();

            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            // 禁用按钮防止重复点击
            runRoadDamageBtn->setEnabled(false);
            runRoadDamageBtn->setText("检测中...");

            // 获取参数
            QDoubleSpinBox *confSpinBox = panel->findChild<QDoubleSpinBox *>("spinConfThreshold");
            QDoubleSpinBox *iouSpinBox = panel->findChild<QDoubleSpinBox *>("spinIOUThreshold");
            QSpinBox *sizeSpinBox = panel->findChild<QSpinBox *>("spinImageSize");
            QCheckBox *showLabelsCheck = panel->findChild<QCheckBox *>("chkShowLabels");

            float confThreshold = confSpinBox ? static_cast<float>(confSpinBox->value()) : 0.25f;
            float iouThreshold = iouSpinBox ? static_cast<float>(iouSpinBox->value()) : 0.45f;
            int imageSize = sizeSpinBox ? sizeSpinBox->value() : 640;

            // 保存显示设置
            m_showLabels = showLabelsCheck ? showLabelsCheck->isChecked() : true;

            runRoadDamageDetection(imagePath, confThreshold, iouThreshold, imageSize);

            // 清理临时文件
            cleanupTempImage();

            // 恢复按钮状态
            runRoadDamageBtn->setEnabled(true);
            runRoadDamageBtn->setText("执行病害检测");
        });
    }

    // ========== 井盖病害检测按钮 ==========
    QPushButton *runManholeCoverBtn = panel->findChild<QPushButton *>("btnRunManholeCover");
    if (runManholeCoverBtn) {
        connect(runManholeCoverBtn, &QPushButton::clicked, this, [this, panel, runManholeCoverBtn]() {
            // 获取当前显示的图像用于推理（可能是处理后的图像）
            QString imagePath = getCurrentImageForInference();

            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            // 禁用按钮防止重复点击
            runManholeCoverBtn->setEnabled(false);
            runManholeCoverBtn->setText("检测中...");

            // 获取参数
            QDoubleSpinBox *confSpinBox = panel->findChild<QDoubleSpinBox *>("spinConfThreshold");
            QDoubleSpinBox *iouSpinBox = panel->findChild<QDoubleSpinBox *>("spinIOUThreshold");
            QSpinBox *sizeSpinBox = panel->findChild<QSpinBox *>("spinImageSize");
            QCheckBox *showLabelsCheck = panel->findChild<QCheckBox *>("chkShowLabels");

            float confThreshold = confSpinBox ? static_cast<float>(confSpinBox->value()) : 0.25f;
            float iouThreshold = iouSpinBox ? static_cast<float>(iouSpinBox->value()) : 0.45f;
            int imageSize = sizeSpinBox ? sizeSpinBox->value() : 640;

            // 保存显示设置
            m_showLabels = showLabelsCheck ? showLabelsCheck->isChecked() : true;

            runManholeCoverDamageDetection(imagePath, confThreshold, iouThreshold, imageSize);

            // 清理临时文件
            cleanupTempImage();

            // 恢复按钮状态
            runManholeCoverBtn->setEnabled(true);
            runManholeCoverBtn->setText("执行井盖检测");
        });
    }

    // ========== 小样本分类按钮 ==========
    QPushButton *runFewShotBtn = panel->findChild<QPushButton *>("btnRunFewShotClassification");
    if (runFewShotBtn) {
        connect(runFewShotBtn, &QPushButton::clicked, this, [this, panel, runFewShotBtn]() {
            // 获取当前显示的图像用于推理（可能是处理后的图像）
            QString imagePath = getCurrentImageForInference();

            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            // 禁用按钮防止重复点击
            runFewShotBtn->setEnabled(false);
            runFewShotBtn->setText("分类中...");

            // 获取参数
            QSpinBox *nWaySpinBox = panel->findChild<QSpinBox *>("spinNWay");
            QSpinBox *nShotSpinBox = panel->findChild<QSpinBox *>("spinNShot");
            QSpinBox *nQuerySpinBox = panel->findChild<QSpinBox *>("spinNQuery");
            QSpinBox *sizeSpinBox = panel->findChild<QSpinBox *>("spinImageSize");

            int nWay = nWaySpinBox ? nWaySpinBox->value() : 5;
            int nShot = nShotSpinBox ? nShotSpinBox->value() : 5;
            int nQuery = nQuerySpinBox ? nQuerySpinBox->value() : 15;
            int imageSize = sizeSpinBox ? sizeSpinBox->value() : 84;

            runFewShotClassification(imagePath, nWay, nShot, nQuery, imageSize);

            // 清理临时文件
            cleanupTempImage();

            // 恢复按钮状态
            runFewShotBtn->setEnabled(true);
            runFewShotBtn->setText("执行小样本分类");
        });
    }

    // ========== 小样本分类详细说明按钮 ==========
    QPushButton *detailBtn = panel->findChild<QPushButton *>("btnFSLDetailInfo");
    if (detailBtn) {
        connect(detailBtn, &QPushButton::clicked, this, [this]() {
            showFSLInfoDialog();
        });
    }

    // ========== 图像增强按钮 ==========
    QPushButton *runEnhanceBtn = panel->findChild<QPushButton *>("btnRunEnhancement");
    if (runEnhanceBtn) {
        connect(runEnhanceBtn, &QPushButton::clicked, this, [this, panel]() {
            // 获取当前显示的图像用于处理（可能是已处理过的图像）
            QString imagePath = getCurrentImageForInference();
            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            QSlider *brightnessSlider = panel->findChild<QSlider *>("sliderBrightness");
            QSlider *contrastSlider = panel->findChild<QSlider *>("sliderContrast");
            QSlider *satSlider = panel->findChild<QSlider *>("sliderSaturation");
            QSlider *sharpSlider = panel->findChild<QSlider *>("sliderSharpness");

            int brightness = brightnessSlider ? brightnessSlider->value() : 0;
            int contrast = contrastSlider ? contrastSlider->value() : 0;
            int saturation = satSlider ? satSlider->value() : 0;
            int sharpness = sharpSlider ? sharpSlider->value() : 0;

            runImageEnhancement(imagePath, brightness, contrast, saturation, sharpness);

            // 清理临时文件
            cleanupTempImage();
        });
    }

    // ========== 图像去噪按钮 ==========
    QPushButton *runDenoiseBtn = panel->findChild<QPushButton *>("btnRunDenoising");
    if (runDenoiseBtn) {
        connect(runDenoiseBtn, &QPushButton::clicked, this, [this, panel]() {
            // 获取当前显示的图像用于处理
            QString imagePath = getCurrentImageForInference();
            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            QComboBox *methodCombo = panel->findChild<QComboBox *>("cmbDenoiseMethod");
            QSpinBox *kernelSpinBox = panel->findChild<QSpinBox *>("spinKernelSize");
            QDoubleSpinBox *sigmaSpinBox = panel->findChild<QDoubleSpinBox *>("spinSigma");

            int method = methodCombo ? methodCombo->currentIndex() : 0;
            int kernelSize = kernelSpinBox ? kernelSpinBox->value() : 3;
            double sigma = sigmaSpinBox ? sigmaSpinBox->value() : 1.0;

            runImageDenoising(imagePath, method, kernelSize, sigma);

            // 清理临时文件
            cleanupTempImage();
        });
    }

    // ========== 边缘检测按钮 ==========
    QPushButton *runEdgeBtn = panel->findChild<QPushButton *>("btnRunEdgeDetection");
    if (runEdgeBtn) {
        connect(runEdgeBtn, &QPushButton::clicked, this, [this, panel]() {
            // 获取当前显示的图像用于处理
            QString imagePath = getCurrentImageForInference();
            if (imagePath.isEmpty()) {
                emit logMessage("请先打开一张图像");
                return;
            }

            QComboBox *methodCombo = panel->findChild<QComboBox *>("cmbEdgeMethod");
            QDoubleSpinBox *threshold1SpinBox = panel->findChild<QDoubleSpinBox *>("spinCannyThreshold1");
            QDoubleSpinBox *threshold2SpinBox = panel->findChild<QDoubleSpinBox *>("spinCannyThreshold2");
            QSpinBox *apertureSpinBox = panel->findChild<QSpinBox *>("spinApertureSize");

            int method = methodCombo ? methodCombo->currentIndex() : 0;
            double threshold1 = threshold1SpinBox ? threshold1SpinBox->value() : 100.0;
            double threshold2 = threshold2SpinBox ? threshold2SpinBox->value() : 200.0;
            int apertureSize = apertureSpinBox ? apertureSpinBox->value() : 3;

            runEdgeDetection(imagePath, method, threshold1, threshold2, apertureSize);

            // 清理临时文件
            cleanupTempImage();
        });
    }
}

void TaskController::onDetectionCompleted(const Utils::DetectionResult &result)
{
    qDebug() << "onDetectionCompleted called, success:" << result.success
             << "detections:" << result.detections.size();

    // 转发信号给外部监听者
    emit detectionCompleted(result);

    // 只在启用时显示结果对话框（批量处理时会禁用）
    if (m_showResultDialog) {
        showResultDialog(result);
    }
}

void TaskController::showResultDialog(const Utils::DetectionResult &result)
{
    qDebug() << "showResultDialog called";

    if (!result.success) {
        emit logMessage(tr("检测失败: %1").arg(result.message));
        return;
    }

    if (!m_resultDialog) {
        m_resultDialog = new Views::DetectionResultDialog(nullptr);
    }

    // 获取当前显示的图片（从 ImageView 获取，确保是处理后的图片）
    QPixmap pixmap;
    ::ImageView *imageView = getCurrentImageView();
    if (imageView) {
        pixmap = imageView->pixmap();
        qDebug() << "Got pixmap from ImageView, null:" << pixmap.isNull() << "size:" << pixmap.size();
    }

    // 如果无法从 ImageView 获取，尝试从缓存的 pixmap 获取
    if (pixmap.isNull() && !m_currentPixmap.isNull()) {
        pixmap = m_currentPixmap;
        qDebug() << "Got pixmap from cache, null:" << pixmap.isNull();
    }

    // 如果无法从 ImageView 获取，尝试从 TabController 获取
    if (pixmap.isNull() && m_tabController) {
        pixmap = m_tabController->currentPixmap();
        qDebug() << "Got pixmap from TabController, null:" << pixmap.isNull();
    }

    // 如果仍然无法获取，尝试从文件路径加载
    if (pixmap.isNull()) {
        QString imagePath = getCurrentImagePath();
        qDebug() << "Trying to load from path:" << imagePath << "exists:" << QFile::exists(imagePath);
        if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
            pixmap.load(imagePath);
        }
    }

    qDebug() << "Final pixmap for result display, null:" << pixmap.isNull() << "size:" << pixmap.size();

    if (!pixmap.isNull()) {
        // 根据任务类型使用不同的显示方式
        if (m_currentTask == Models::CVTask::SemanticSegmentation) {
            // 使用蒙版显示分割结果
            m_resultDialog->setSegmentationResult(pixmap, result, m_maskAlpha, m_showBoxes, m_showLabels);
            emit logMessage(tr("语义分割结果已显示"));
        } else {
            // 使用边界框显示检测结果
            m_resultDialog->setResult(pixmap, result, m_showLabels);
            emit logMessage(tr("检测结果已显示"));
        }
        m_resultDialog->show();
        m_resultDialog->raise();
        m_resultDialog->activateWindow();
        return;
    }

    // 如果无法加载图像，显示错误
    emit logMessage(tr("无法显示检测结果: 图像未加载或路径无效"));
}

void TaskController::runImageEnhancement(const QString &imagePath, int brightness,
                                          int contrast, int saturation, int sharpness)
{
    emit logMessage(tr("执行图像增强..."));

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        emit logMessage(tr("无法加载图像: %1").arg(imagePath));
        return;
    }

    m_currentImagePath = imagePath;

    Utils::ProcessResult result = m_imageProcessService->enhanceImage(
        pixmap.toImage(), brightness, contrast, saturation, sharpness);

    if (result.success) {
        // 显示处理后的图像 - 使用对比视图
        if (!m_resultDialog) {
            m_resultDialog = new Views::DetectionResultDialog(nullptr);
        }

        QPixmap resultPixmap = QPixmap::fromImage(result.processedImage);
        m_resultDialog->setImageProcessResult(pixmap, resultPixmap, tr("图像增强"), result.processTime);
        m_resultDialog->show();
        m_resultDialog->raise();
        m_resultDialog->activateWindow();

        emit logMessage(result.message);
        emit imageProcessCompleted(result);
    } else {
        emit logMessage(tr("图像增强失败: %1").arg(result.message));
    }
}

void TaskController::runImageDenoising(const QString &imagePath, int method,
                                        int kernelSize, double sigma)
{
    emit logMessage(tr("执行图像去噪..."));

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        emit logMessage(tr("无法加载图像: %1").arg(imagePath));
        return;
    }

    m_currentImagePath = imagePath;

    Utils::ImageProcessService::DenoiseMethod denoiseMethod =
        static_cast<Utils::ImageProcessService::DenoiseMethod>(method);

    Utils::ProcessResult result = m_imageProcessService->denoiseImage(
        pixmap.toImage(), denoiseMethod, kernelSize, sigma);

    if (result.success) {
        // 显示处理后的图像 - 使用对比视图
        if (!m_resultDialog) {
            m_resultDialog = new Views::DetectionResultDialog(nullptr);
        }

        QPixmap resultPixmap = QPixmap::fromImage(result.processedImage);
        m_resultDialog->setImageProcessResult(pixmap, resultPixmap, tr("图像去噪"), result.processTime);
        m_resultDialog->show();
        m_resultDialog->raise();
        m_resultDialog->activateWindow();

        emit logMessage(result.message);
        emit imageProcessCompleted(result);
    } else {
        emit logMessage(tr("图像去噪失败: %1").arg(result.message));
    }
}

void TaskController::runEdgeDetection(const QString &imagePath, int method,
                                       double threshold1, double threshold2, int apertureSize)
{
    emit logMessage(tr("执行边缘检测..."));

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        emit logMessage(tr("无法加载图像: %1").arg(imagePath));
        return;
    }

    m_currentImagePath = imagePath;

    Utils::ImageProcessService::EdgeMethod edgeMethod =
        static_cast<Utils::ImageProcessService::EdgeMethod>(method);

    Utils::ProcessResult result = m_imageProcessService->detectEdges(
        pixmap.toImage(), edgeMethod, threshold1, threshold2, apertureSize);

    if (result.success) {
        if (!m_resultDialog) {
            m_resultDialog = new Views::DetectionResultDialog(nullptr);
        }

        QPixmap resultPixmap = QPixmap::fromImage(result.processedImage);
        m_resultDialog->setImageProcessResult(pixmap, resultPixmap, tr("边缘检测"), result.processTime);
        m_resultDialog->show();
        m_resultDialog->raise();
        m_resultDialog->activateWindow();

        emit logMessage(result.message);
        emit imageProcessCompleted(result);
    } else {
        emit logMessage(tr("边缘检测失败: %1").arg(result.message));
    }
}

void TaskController::runClassification(const QString &imagePath, int topK)
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("服务未运行，请先启动服务");
        return;
    }

    emit logMessage(QString("执行图像分类"));

    // 同步调用分类
    Utils::ClassificationResultList result = m_dlService->classify(imagePath, topK);

    if (result.success) {
        // 显示分类结果
        if (!m_resultDialog) {
            m_resultDialog = new Views::DetectionResultDialog(nullptr);
        }

        // 获取当前显示的图像用于显示（从 ImageView 获取，确保是处理后的图片）
        QPixmap pixmap;
        ::ImageView *imageView = getCurrentImageView();
        if (imageView) {
            pixmap = imageView->pixmap();
        }

        // 如果无法从 ImageView 获取，尝试从文件加载
        if (pixmap.isNull()) {
            pixmap.load(imagePath);
        }

        m_resultDialog->setClassificationResult(pixmap, result);
        m_resultDialog->show();
        m_resultDialog->raise();
        m_resultDialog->activateWindow();

        emit logMessage(result.message);
    } else {
        emit logMessage(tr("分类失败: %1").arg(result.message));
    }
}

void TaskController::runKeypointDetection(const QString &imagePath, float confThreshold,
                                           float iouThreshold, int imageSize)
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("服务未运行，请先启动服务");
        return;
    }

    emit logMessage(QString("执行关键点检测"));

    // 同步调用关键点检测
    Utils::KeypointResult result = m_dlService->keypoint(imagePath, confThreshold, iouThreshold, imageSize);

    if (result.success) {
        // 显示关键点检测结果
        if (!m_resultDialog) {
            m_resultDialog = new Views::DetectionResultDialog(nullptr);
        }

        // 获取当前显示的图像用于显示（从 ImageView 获取，确保是处理后的图片）
        QPixmap pixmap;
        ::ImageView *imageView = getCurrentImageView();
        if (imageView) {
            pixmap = imageView->pixmap();
        }

        // 如果无法从 ImageView 获取，尝试从文件加载
        if (pixmap.isNull()) {
            pixmap.load(imagePath);
        }

        m_resultDialog->setKeypointResult(pixmap, result, m_showBoxes, m_showLabels);
        m_resultDialog->show();
        m_resultDialog->raise();
        m_resultDialog->activateWindow();

        emit logMessage(result.message);
    } else {
        emit logMessage(tr("关键点检测失败: %1").arg(result.message));
    }
}

void TaskController::runRoadDamageDetection(const QString &imagePath, float confThreshold,
                                             float iouThreshold, int imageSize)
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("服务未运行，请先启动服务");
        return;
    }

    // 保存当前图像路径，用于显示结果
    m_currentImagePath = imagePath;

    emit logMessage(QString("执行道路病害检测: %1").arg(imagePath));

    // 同步调用检测（使用 detect 方法，与目标检测相同）
    Utils::DetectionResult result = m_dlService->detect(
        imagePath, confThreshold, iouThreshold, imageSize);

    // 注意：信号 detectionCompleted 会在 detect() 内部发射，
    // 触发 onDetectionCompleted() 显示结果对话框
}

void TaskController::runManholeCoverDamageDetection(const QString &imagePath, float confThreshold,
                                                     float iouThreshold, int imageSize)
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("服务未运行，请先启动服务");
        return;
    }

    // 保存当前图像路径，用于显示结果
    m_currentImagePath = imagePath;

    emit logMessage(QString("执行井盖病害检测: %1").arg(imagePath));

    // 同步调用检测（使用 detect 方法，与目标检测相同）
    Utils::DetectionResult result = m_dlService->detect(
        imagePath, confThreshold, iouThreshold, imageSize);

    // 注意：信号 detectionCompleted 会在 detect() 内部发射，
    // 触发 onDetectionCompleted() 显示结果对话框
}

void TaskController::runFewShotClassification(const QString &imagePath, int nWay,
                                               int nShot, int nQuery, int imageSize)
{
    if (!m_dlService) {
        emit logMessage("服务未初始化");
        return;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("服务未运行，请先启动服务");
        return;
    }

    // 保存当前图像路径，用于显示结果
    m_currentImagePath = imagePath;

    emit logMessage(QString("执行遥感影像小样本分类: %1").arg(imagePath));
    emit logMessage(QString("参数: N-way=%1, N-shot=%2, N-query=%3, ImageSize=%4")
                    .arg(nWay).arg(nShot).arg(nQuery).arg(imageSize));

    // 同步调用小样本分类
    Utils::ClassificationResultList result = m_dlService->fewShotClassify(
        imagePath, nWay, nShot, nQuery, imageSize);

    if (result.success) {
        // 显示小样本分类结果
        if (!m_resultDialog) {
            m_resultDialog = new Views::DetectionResultDialog(nullptr);
        }

        // 获取当前显示的图像用于显示（从 ImageView 获取，确保是处理后的图片）
        QPixmap pixmap;
        ::ImageView *imageView = getCurrentImageView();
        if (imageView) {
            pixmap = imageView->pixmap();
        }

        // 如果无法从 ImageView 获取，尝试从文件加载
        if (pixmap.isNull()) {
            pixmap.load(imagePath);
        }

        m_resultDialog->setFewShotClassificationResult(pixmap, result, nWay, nShot);
        m_resultDialog->show();
        m_resultDialog->raise();
        m_resultDialog->activateWindow();

        emit logMessage(result.message);
    } else {
        emit logMessage(tr("小样本分类失败: %1").arg(result.message));
    }
}

void TaskController::showFSLInfoDialog()
{
    // 创建详细说明对话框
    QDialog *dialog = new QDialog(nullptr);
    dialog->setWindowTitle(tr("遥感影像小样本分类 - 详细说明"));
    dialog->setMinimumSize(550, 450);
    dialog->setStyleSheet(
        "QDialog { background-color: #f8f9fa; }"
        "QLabel { color: #333333; }"
        "QGroupBox { font-weight: bold; border: 1px solid #dee2e6; border-radius: 6px; "
        "            margin-top: 12px; padding-top: 12px; background-color: white; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #0066cc; }"
        "QPushButton { background-color: #0066cc; color: white; border: none; padding: 8px 20px; "
        "              border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0052a3; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 标题
    QLabel *titleLabel = new QLabel(tr("🛰 遥感影像小样本分类 (Few-Shot Learning)"));
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0066cc; padding-bottom: 5px;");
    mainLayout->addWidget(titleLabel);

    // 滚动区域
    QScrollArea *scrollArea = new QScrollArea(dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(15);

    // 任务介绍
    QGroupBox *introGroup = new QGroupBox(tr("📖 任务介绍"));
    QVBoxLayout *introLayout = new QVBoxLayout(introGroup);
    QLabel *introLabel = new QLabel(
        tr("遥感影像小样本分类是基于原型网络（Prototypical Networks）的小样本学习方法，"
           "旨在解决遥感影像场景分类中标注数据稀缺的问题。\n\n"
           "通过 N-way-K-shot 的学习范式，模型能够从少量标注样本中快速学习新类别，"
           "适用于 30 类典型遥感场景的分类任务。")
    );
    introLabel->setWordWrap(true);
    introLabel->setStyleSheet("font-size: 12px; line-height: 1.6;");
    introLayout->addWidget(introLabel);
    contentLayout->addWidget(introGroup);

    // 参数说明
    QGroupBox *paramGroup = new QGroupBox(tr("⚙️ 参数说明"));
    QVBoxLayout *paramLayout = new QVBoxLayout(paramGroup);
    QLabel *paramLabel = new QLabel(
        tr("• N-way：每轮任务中的类别数量（2-20）\n"
           "• N-shot：每个类别的支持样本数（1-20）\n"
           "• N-query：每个类别的查询样本数（1-50）\n"
           "• 输入尺寸：模型输入图像尺寸（64-224，推荐84）\n\n"
           "例如：5-way-5-shot 表示从5个类别中各选5个样本进行学习，"
           "然后对查询样本进行分类。")
    );
    paramLabel->setWordWrap(true);
    paramLabel->setStyleSheet("font-size: 12px; line-height: 1.6;");
    paramLayout->addWidget(paramLabel);
    contentLayout->addWidget(paramGroup);

    // MEET-FSL 数据集类别（30类）
    QGroupBox *classGroup = new QGroupBox(tr("📊 MEET-FSL 数据集类别（30类）"));
    QVBoxLayout *classLayout = new QVBoxLayout(classGroup);

    // 创建两列布局
    QHBoxLayout *columnsLayout = new QHBoxLayout();
    QVBoxLayout *leftCol = new QVBoxLayout();
    QVBoxLayout *rightCol = new QVBoxLayout();

    QStringList classes = {
        "1. 航空站 (Airport)", "2. 桥梁 (Bridge)", "3. 中央商务区 (CBD)",
        "4. 教堂 (Church)", "5. 商业区 (Commercial)", "6. 密集住宅区",
        "7. 工业区 (Industrial)", "8. 中等密度住宅区", "9. 低密度住宅区",
        "10. 农场 (Farm)", "11. 森林 (Forest)", "12. 草地 (Meadow)",
        "13. 湿地 (Wetland)", "14. 水体 (Water)", "15. 湿地植被 (Marsh)",
        "16. 海滩 (Beach)", "17. 港口 (Port)", "18. 广场 (Square)",
        "19. 高尔夫球场", "20. 游乐场 (Playground)", "21. 大学 (University)",
        "22. 医院 (Hospital)", "23. 机场航站楼", "24. 停机坪 (Runway)",
        "25. 高速公路 (Freeway)", "26. 铁路 (Railway)", "27. 停车场 (Parking)",
        "28. 工地 (Construction)", "29. 灾害 (Disaster)", "30. 立交桥 (Overpass)"
    };

    for (int i = 0; i < classes.size(); ++i) {
        QLabel *label = new QLabel(classes[i]);
        label->setStyleSheet("font-size: 11px; color: #495057; padding: 1px 0;");
        if (i < 15) {
            leftCol->addWidget(label);
        } else {
            rightCol->addWidget(label);
        }
    }
    leftCol->addStretch();
    rightCol->addStretch();

    columnsLayout->addLayout(leftCol);
    columnsLayout->addLayout(rightCol);
    classLayout->addLayout(columnsLayout);
    contentLayout->addWidget(classGroup);

    // 使用提示
    QGroupBox *tipGroup = new QGroupBox(tr("💡 使用提示"));
    QVBoxLayout *tipLayout = new QVBoxLayout(tipGroup);
    QLabel *tipLabel = new QLabel(
        tr("• 建议参数设置：N-way=5, N-shot=5, 适用于大多数场景\n"
           "• 当类别差异较大时，可适当增加 N-way 到 10\n"
           "• 当样本质量较低时，建议减少 N-shot 到 1-3\n"
           "• 输入尺寸保持默认值 84 即可，无需调整\n"
           "• 模型会在后台执行多轮 episode 采样来提高分类稳定性")
    );
    tipLabel->setWordWrap(true);
    tipLabel->setStyleSheet("font-size: 12px; line-height: 1.6; color: #856404; background-color: #fff3cd; "
                            "padding: 10px; border-radius: 4px;");
    tipLayout->addWidget(tipLabel);
    contentLayout->addWidget(tipGroup);

    contentLayout->addStretch();
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 1);

    // 确定按钮
    QPushButton *okBtn = new QPushButton(tr("确定"));
    okBtn->setFixedWidth(100);
    connect(okBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    dialog->exec();
    delete dialog;
}

} // namespace Controllers
} // namespace GenPreCVSystem
