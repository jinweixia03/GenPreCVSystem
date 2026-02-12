#include "environmentservicewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QTimer>
#include <QCoreApplication>
#include <QDebug>

namespace GenPreCVSystem {
namespace Widgets {

EnvironmentServiceWidget::EnvironmentServiceWidget(QWidget *parent)
    : QWidget(parent)
    , m_envCombo(nullptr)
    , m_lblServiceStatus(nullptr)
    , m_cmbModelSelect(nullptr)
    , m_btnBrowseModel(nullptr)
    , m_lblModelStatus(nullptr)
    , m_yoloService(nullptr)
    , m_currentTask(Models::CVTask::ImageClassification)
{
    setupUI();
    connectSignals();

    // 延迟扫描环境列表
    QTimer::singleShot(100, this, &EnvironmentServiceWidget::scanEnvironments);
}

EnvironmentServiceWidget::~EnvironmentServiceWidget()
{
}

void EnvironmentServiceWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // ========== Python 环境与服务状态 ==========
    QGroupBox *envGroup = new QGroupBox("Python 环境", this);
    QVBoxLayout *envLayout = new QVBoxLayout(envGroup);

    // 环境下拉框（无刷新按钮）
    m_envCombo = new QComboBox(this);
    m_envCombo->setObjectName("cmbPythonEnv");
    m_envCombo->setPlaceholderText("正在扫描环境...");
    envLayout->addWidget(m_envCombo);

    // 服务状态标签
    m_lblServiceStatus = new QLabel("正在扫描环境...", this);
    m_lblServiceStatus->setObjectName("lblServiceStatus");
    m_lblServiceStatus->setStyleSheet("color: #888; font-size: 11px;");
    m_lblServiceStatus->setWordWrap(true);
    envLayout->addWidget(m_lblServiceStatus);

    mainLayout->addWidget(envGroup);

    // ========== 模型设置 ==========
    QGroupBox *modelGroup = new QGroupBox("模型设置", this);
    QVBoxLayout *modelLayout = new QVBoxLayout(modelGroup);

    // 模型下拉选择 + 浏览按钮（水平布局）
    QHBoxLayout *modelSelectLayout = new QHBoxLayout();
    m_cmbModelSelect = new QComboBox(this);
    m_cmbModelSelect->setObjectName("cmbModelSelect");
    m_cmbModelSelect->setPlaceholderText("选择模型...");
    modelSelectLayout->addWidget(m_cmbModelSelect, 1);

    m_btnBrowseModel = new QPushButton("浏览...", this);
    m_btnBrowseModel->setObjectName("btnBrowseModel");
    m_btnBrowseModel->setFixedWidth(60);
    modelSelectLayout->addWidget(m_btnBrowseModel);

    modelLayout->addLayout(modelSelectLayout);

    // 模型状态标签（左对齐，与服务状态一致）
    m_lblModelStatus = new QLabel("模型状态: 未加载", this);
    m_lblModelStatus->setObjectName("lblModelStatus");
    m_lblModelStatus->setStyleSheet("color: #666; font-size: 11px;");
    m_lblModelStatus->setWordWrap(true);
    modelLayout->addWidget(m_lblModelStatus);

    mainLayout->addWidget(modelGroup);
}

void EnvironmentServiceWidget::connectSignals()
{
    // 环境选择变化
    connect(m_envCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnvironmentServiceWidget::onEnvironmentChanged);

    // 浏览模型
    connect(m_btnBrowseModel, &QPushButton::clicked, this, &EnvironmentServiceWidget::onBrowseModel);

    // 模型选择变化
    connect(m_cmbModelSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnvironmentServiceWidget::onModelSelectionChanged);
}

void EnvironmentServiceWidget::setYOLOService(Utils::YOLOService *service)
{
    m_yoloService = service;

    // 保存当前环境路径
    if (m_yoloService) {
        m_savedEnvPath = m_yoloService->currentEnvironmentPath();
    }

    // 更新服务状态显示
    updateServiceStatus();
}

void EnvironmentServiceWidget::updateServiceStatus()
{
    if (!m_yoloService) {
        m_lblServiceStatus->setText("服务状态: 未初始化");
        m_lblServiceStatus->setStyleSheet("color: #cc0000; font-size: 11px;");
        return;
    }

    if (m_yoloService->isRunning()) {
        m_lblServiceStatus->setText("服务状态: 运行中 ✓");
        m_lblServiceStatus->setStyleSheet("color: #228B22; font-size: 11px; font-weight: bold;");
    } else {
        m_lblServiceStatus->setText("服务状态: 未启动");
        m_lblServiceStatus->setStyleSheet("color: #666; font-size: 11px;");
    }
}

void EnvironmentServiceWidget::updateModelStatus()
{
    if (!m_yoloService || !m_yoloService->isRunning()) {
        m_lblModelStatus->setText("模型状态: 服务未启动");
        m_lblModelStatus->setStyleSheet("color: #888; font-size: 11px;");
        return;
    }

    if (m_currentModelPath.isEmpty()) {
        m_lblModelStatus->setText("模型状态: 未选择");
        m_lblModelStatus->setStyleSheet("color: #888; font-size: 11px;");
        return;
    }

    // 检查当前选择的模型是否已加载
    if (m_yoloService->isModelLoaded() && m_currentModelPath == m_loadedModelPath) {
        QString modelName = QFileInfo(m_currentModelPath).fileName();
        m_lblModelStatus->setText(QString("模型: %1 ✓").arg(modelName));
        m_lblModelStatus->setStyleSheet("color: #228B22; font-size: 11px; font-weight: bold;");
    } else {
        m_lblModelStatus->setText("模型状态: 未加载");
        m_lblModelStatus->setStyleSheet("color: #888; font-size: 11px;");
    }
}

void EnvironmentServiceWidget::updateRunButtonState(bool enabled)
{
    Q_UNUSED(enabled);
}

void EnvironmentServiceWidget::scanEnvironments()
{
    m_envCombo->clear();
    m_lblServiceStatus->setText("正在扫描环境...");
    m_lblServiceStatus->setStyleSheet("color: #0066cc; font-size: 11px;");

    // 扫描环境
    QVector<Utils::PythonEnvironment> envs = Utils::YOLOService::scanEnvironments();

    int selectedIndex = -1;
    for (const auto &env : envs) {
        QString displayText = env.name;
        if (env.hasUltralytics) {
            displayText += " ✓";
        }
        displayText += QString(" (%1)").arg(env.type);

        m_envCombo->addItem(displayText, env.path);

        // 设置有 ultralytics 的项的背景色
        if (env.hasUltralytics) {
            int index = m_envCombo->count() - 1;
            m_envCombo->setItemData(index, QColor(100, 180, 100), Qt::BackgroundRole);
        }

        // 检查是否匹配已保存的环境
        if (!m_savedEnvPath.isEmpty() &&
            QDir::cleanPath(env.path) == QDir::cleanPath(m_savedEnvPath)) {
            selectedIndex = m_envCombo->count() - 1;
        }
    }

    // 如果找到匹配的已保存环境，自动选择它
    if (selectedIndex >= 0) {
        m_envCombo->setCurrentIndex(selectedIndex);
    } else if (!envs.isEmpty()) {
        // 默认选择第一个有 ultralytics 的环境
        for (int i = 0; i < m_envCombo->count(); ++i) {
            QString text = m_envCombo->itemText(i);
            if (text.contains("✓")) {
                m_envCombo->setCurrentIndex(i);
                break;
            }
        }
        // 如果没有找到有 ultralytics 的环境，选择第一个
        if (m_envCombo->currentIndex() < 0 && m_envCombo->count() > 0) {
            m_envCombo->setCurrentIndex(0);
        }
    } else {
        m_lblServiceStatus->setText("未找到 Python 环境，请检查安装");
        m_lblServiceStatus->setStyleSheet("color: #cc0000; font-size: 11px;");
    }
}

void EnvironmentServiceWidget::tryAutoStartService()
{
    if (!m_yoloService || m_yoloService->isRunning()) {
        // 如果服务已运行，尝试加载模型
        if (m_yoloService && m_yoloService->isRunning()) {
            tryAutoLoadModel();
        }
        return;
    }

    // 检查当前选择的环境是否有 ultralytics
    int index = m_envCombo->currentIndex();
    if (index < 0) return;

    QString envText = m_envCombo->itemText(index);
    if (!envText.contains("✓")) {
        m_lblServiceStatus->setText("当前环境缺少 ultralytics，请选择带 ✓ 的环境");
        m_lblServiceStatus->setStyleSheet("color: #FF8C00; font-size: 11px;");
        return;
    }

    // 自动启动服务
    m_lblServiceStatus->setText("正在启动服务...");
    m_lblServiceStatus->setStyleSheet("color: #0066cc; font-size: 11px;");

    bool success = m_yoloService->start();

    if (success) {
        m_lblServiceStatus->setText("服务状态: 运行中 ✓");
        m_lblServiceStatus->setStyleSheet("color: #228B22; font-size: 11px; font-weight: bold;");

        emit logMessage("YOLO 服务已自动启动");
        emit serviceStarted();

        // 尝试自动加载模型
        QTimer::singleShot(100, this, &EnvironmentServiceWidget::tryAutoLoadModel);
    } else {
        m_lblServiceStatus->setText("服务启动失败");
        m_lblServiceStatus->setStyleSheet("color: #cc0000; font-size: 11px;");
        emit logMessage("YOLO 服务启动失败");
    }
}

void EnvironmentServiceWidget::tryAutoLoadModel()
{
    if (!m_yoloService || !m_yoloService->isRunning()) {
        updateModelStatus();
        return;
    }

    // 如果没有模型路径，不执行
    if (m_currentModelPath.isEmpty()) {
        updateModelStatus();
        return;
    }

    // 如果当前模型已经加载，不重复加载
    if (!m_loadedModelPath.isEmpty() && m_currentModelPath == m_loadedModelPath && m_yoloService->isModelLoaded()) {
        updateModelStatus();
        return;
    }

    m_lblModelStatus->setText("正在加载模型...");
    m_lblModelStatus->setStyleSheet("color: #0066cc; font-size: 11px;");

    bool success = m_yoloService->loadModel(m_currentModelPath);

    if (success) {
        m_loadedModelPath = m_currentModelPath;  // 记录已加载的模型路径
        updateModelStatus();
        emit logMessage(QString("模型加载成功: %1").arg(QFileInfo(m_currentModelPath).fileName()));
        emit modelLoaded(m_currentModelPath);
    } else {
        m_loadedModelPath.clear();  // 加载失败，清除已加载路径
        m_lblModelStatus->setText("模型状态: 加载失败");
        m_lblModelStatus->setStyleSheet("color: #cc0000; font-size: 11px;");
        emit logMessage("模型加载失败");
    }
}

void EnvironmentServiceWidget::onEnvironmentChanged(int index)
{
    if (index < 0) return;

    QString envPath = m_envCombo->itemData(index).toString();
    QString envName = m_envCombo->itemText(index);

    if (m_yoloService) {
        // 如果服务正在运行，先停止
        if (m_yoloService->isRunning()) {
            m_yoloService->stop();
            m_loadedModelPath.clear();  // 清除已加载的模型路径
            emit serviceStopped();
        }

        m_yoloService->setEnvironmentPath(envPath);
        m_savedEnvPath = envPath;
        emit logMessage(QString("已选择环境: %1").arg(envName));
        emit environmentChanged(envPath);
    }

    // 更新模型状态
    updateModelStatus();

    // 自动启动服务
    QTimer::singleShot(100, this, &EnvironmentServiceWidget::tryAutoStartService);
}

void EnvironmentServiceWidget::onBrowseModel()
{
    QString modelPath = QFileDialog::getOpenFileName(
        this,
        "选择 YOLO 模型文件",
        QString(),
        "模型文件 (*.pt *.pth *.onnx);;所有文件 (*)"
    );

    if (!modelPath.isEmpty()) {
        m_currentModelPath = modelPath;
        // 只显示文件名，不显示完整路径
        QString fileName = QFileInfo(modelPath).fileName();

        // 添加到下拉框（如果不存在）
        bool found = false;
        for (int i = 0; i < m_cmbModelSelect->count(); ++i) {
            if (m_cmbModelSelect->itemData(i).toString() == modelPath) {
                m_cmbModelSelect->setCurrentIndex(i);
                found = true;
                break;
            }
        }

        if (!found) {
            m_cmbModelSelect->addItem(fileName, modelPath);
            m_cmbModelSelect->setCurrentIndex(m_cmbModelSelect->count() - 1);
        }

        emit logMessage(QString("已选择模型: %1").arg(fileName));

        // 更新模型状态
        updateModelStatus();

        // 尝试自动加载模型
        QTimer::singleShot(100, this, &EnvironmentServiceWidget::tryAutoLoadModel);
    }
}

void EnvironmentServiceWidget::onModelSelectionChanged(int index)
{
    if (index < 0) return;

    QString modelPath = m_cmbModelSelect->itemData(index).toString();
    m_currentModelPath = modelPath;
    emit logMessage(QString("已选择模型: %1").arg(m_cmbModelSelect->itemText(index)));

    // 更新模型状态
    updateModelStatus();

    // 尝试自动加载模型
    QTimer::singleShot(100, this, &EnvironmentServiceWidget::tryAutoLoadModel);
}

void EnvironmentServiceWidget::updateModelList(Models::CVTask task)
{
    m_currentTask = task;

    // 保存当前选择的模型
    QString currentModelName = m_cmbModelSelect->currentText();

    m_cmbModelSelect->clear();

    // 获取模型目录
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    // 尝试找到 resources/models 目录
    QStringList possiblePaths = {
        appDir + "/../src/resources/models",
        appDir + "/src/resources/models",
        appDir + "/../../src/resources/models",
        appDir + "/resources/models",
    };

    // 尝试从构建目录找到源目录
    if (dir.cdUp()) {
        possiblePaths.prepend(dir.absolutePath() + "/src/resources/models");
    }

    QString basePath;
    for (const QString &path : possiblePaths) {
        QString normalized = QDir::cleanPath(path);
        if (QDir(normalized).exists()) {
            basePath = normalized;
            break;
        }
    }

    if (basePath.isEmpty()) {
        basePath = QDir::cleanPath(appDir + "/../src/resources/models");
    }

    // 根据任务类型返回子目录
    QString modelDir;
    switch (task) {
        case Models::CVTask::ObjectDetection:
            modelDir = basePath + "/detection";
            break;
        case Models::CVTask::SemanticSegmentation:
            modelDir = basePath + "/segmentation";
            break;
        case Models::CVTask::ImageClassification:
            modelDir = basePath + "/classification";
            break;
        case Models::CVTask::KeyPointDetection:
            modelDir = basePath + "/keypoint";
            break;
        default:
            modelDir = basePath;
            break;
    }

    QDir dirObj(modelDir);
    if (!dirObj.exists()) {
        m_cmbModelSelect->setPlaceholderText("未找到模型目录");
        m_lblModelStatus->setText("模型状态: 目录不存在");
        m_lblModelStatus->setStyleSheet("color: #cc0000; font-size: 11px;");
        emit logMessage(QString("模型目录不存在: %1").arg(modelDir));
        return;
    }

    // 支持的模型文件扩展名
    QStringList filters;
    filters << "*.pt" << "*.pth" << "*.onnx" << "*.torchscript";

    QFileInfoList fileList = dirObj.entryInfoList(filters, QDir::Files | QDir::Readable);

    if (fileList.isEmpty()) {
        m_cmbModelSelect->setPlaceholderText("未找到模型文件");
        m_lblModelStatus->setText("模型状态: 无可用模型");
        m_lblModelStatus->setStyleSheet("color: #888; font-size: 11px;");
        emit logMessage(QString("未在 %1 目录找到模型文件").arg(modelDir));
    } else {
        int selectIndex = -1;
        for (int i = 0; i < fileList.size(); ++i) {
            const QFileInfo &fileInfo = fileList[i];
            // 只显示文件名
            m_cmbModelSelect->addItem(fileInfo.fileName(), fileInfo.absoluteFilePath());

            // 尝试恢复之前选择的模型
            if (fileInfo.fileName() == currentModelName) {
                selectIndex = i;
            }
        }

        // 自动选择第一个模型（或恢复之前的选择）
        if (selectIndex >= 0) {
            m_cmbModelSelect->setCurrentIndex(selectIndex);
        } else if (!fileList.isEmpty()) {
            m_cmbModelSelect->setCurrentIndex(0);
        }

        m_lblModelStatus->setText(QString("已找到 %1 个模型").arg(fileList.size()));
        m_lblModelStatus->setStyleSheet("color: #666; font-size: 11px;");
        emit logMessage(QString("已扫描到 %1 个可用模型").arg(fileList.size()));
    }
}

} // namespace Widgets
} // namespace GenPreCVSystem
