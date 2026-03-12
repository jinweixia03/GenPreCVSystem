/**
 * @file environmentservicewidget.cpp
 * @brief 环境服务控件实现
 *
 * 提供 Python 环境和 DL 模型管理的共享控件：
 * - Python 环境选择（自动扫描 conda/venv/system）
 * - 服务状态显示（运行中/未启动）
 * - 模型选择和加载状态
 *
 * 所有 AI 任务共享此控件，切换任务时保持状态
 */

#include "environmentservicewidget.h"
#include "environmentcachemanager.h"
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
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

namespace GenPreCVSystem {
namespace Widgets {

EnvironmentServiceWidget::EnvironmentServiceWidget(QWidget *parent)
    : QWidget(parent)
    , m_envCombo(nullptr)
    , m_btnReloadEnvs(nullptr)
    , m_lblServiceStatus(nullptr)
    , m_cmbModelSelect(nullptr)
    , m_btnBrowseModel(nullptr)
    , m_lblModelStatus(nullptr)
    , m_dlService(nullptr)
    , m_currentTask(Models::CVTask::ImageClassification)
    , m_envScanner(nullptr)
{
    setupUI();
    connectSignals();

    // 创建扫描线程
    m_envScanner = new Utils::EnvironmentScanner(this);
    connect(m_envScanner, &Utils::EnvironmentScanner::scanProgress,
            this, &EnvironmentServiceWidget::onScanProgress);
    connect(m_envScanner, &Utils::EnvironmentScanner::scanCompleted,
            this, &EnvironmentServiceWidget::onScanCompleted);
    connect(m_envScanner, &Utils::EnvironmentScanner::scanFailed,
            this, &EnvironmentServiceWidget::onScanFailed);

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
    QGroupBox *envGroup = new QGroupBox("🐍 Python 环境", this);
    QVBoxLayout *envLayout = new QVBoxLayout(envGroup);

    // 环境下拉框 + 重载按钮（水平布局）
    QHBoxLayout *envSelectLayout = new QHBoxLayout();
    m_envCombo = new QComboBox(this);
    m_envCombo->setObjectName("cmbPythonEnv");
    m_envCombo->setPlaceholderText("正在扫描环境...");
    envSelectLayout->addWidget(m_envCombo, 1);

    m_btnReloadEnvs = new QPushButton("🔄", this);
    m_btnReloadEnvs->setObjectName("btnReloadEnvs");
    m_btnReloadEnvs->setFixedWidth(30);
    m_btnReloadEnvs->setToolTip("重新检测环境");
    envSelectLayout->addWidget(m_btnReloadEnvs);

    envLayout->addLayout(envSelectLayout);

    // 服务状态标签
    m_lblServiceStatus = new QLabel("⏳ 正在扫描环境...", this);
    m_lblServiceStatus->setObjectName("lblServiceStatus");
    m_lblServiceStatus->setStyleSheet("color: #333333; font-size: 11px;");
    m_lblServiceStatus->setWordWrap(true);
    envLayout->addWidget(m_lblServiceStatus);

    mainLayout->addWidget(envGroup);

    // ========== 模型设置 ==========
    QGroupBox *modelGroup = new QGroupBox("🎯 模型设置", this);
    QVBoxLayout *modelLayout = new QVBoxLayout(modelGroup);

    // 模型下拉选择 + 浏览按钮（水平布局）
    QHBoxLayout *modelSelectLayout = new QHBoxLayout();
    m_cmbModelSelect = new QComboBox(this);
    m_cmbModelSelect->setObjectName("cmbModelSelect");
    m_cmbModelSelect->setPlaceholderText("选择模型...");
    modelSelectLayout->addWidget(m_cmbModelSelect, 1);

    m_btnBrowseModel = new QPushButton("📂 浏览...", this);
    m_btnBrowseModel->setObjectName("btnBrowseModel");
    m_btnBrowseModel->setFixedWidth(70);
    modelSelectLayout->addWidget(m_btnBrowseModel);

    modelLayout->addLayout(modelSelectLayout);

    // 模型状态标签（左对齐，与服务状态一致）
    m_lblModelStatus = new QLabel("📦 模型状态: 未加载", this);
    m_lblModelStatus->setObjectName("lblModelStatus");
    m_lblModelStatus->setStyleSheet("color: #333333; font-size: 11px;");
    m_lblModelStatus->setWordWrap(true);
    modelLayout->addWidget(m_lblModelStatus);

    mainLayout->addWidget(modelGroup);
}

void EnvironmentServiceWidget::connectSignals()
{
    // 环境选择变化
    connect(m_envCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnvironmentServiceWidget::onEnvironmentChanged);

    // 重载环境按钮
    connect(m_btnReloadEnvs, &QPushButton::clicked, this, &EnvironmentServiceWidget::onReloadEnvironments);

    // 浏览模型
    connect(m_btnBrowseModel, &QPushButton::clicked, this, &EnvironmentServiceWidget::onBrowseModel);

    // 模型选择变化
    connect(m_cmbModelSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnvironmentServiceWidget::onModelSelectionChanged);
}

void EnvironmentServiceWidget::setDLService(Utils::DLService *service)
{
    m_dlService = service;

    // 保存当前环境路径
    if (m_dlService) {
        m_savedEnvPath = m_dlService->currentEnvironmentPath();
    }

    // 更新服务状态显示
    updateServiceStatus();
}

void EnvironmentServiceWidget::updateServiceStatus()
{
    if (!m_dlService) {
        m_lblServiceStatus->setText("⚠ 服务状态: 未初始化");
        m_lblServiceStatus->setStyleSheet("color: #cc3300; font-size: 11px;");
        return;
    }

    if (m_dlService->isRunning()) {
        m_lblServiceStatus->setText("● 服务状态: 运行中");
        m_lblServiceStatus->setStyleSheet("color: #0066cc; font-size: 11px; font-weight: bold;");
    } else {
        m_lblServiceStatus->setText("○ 服务状态: 未启动");
        m_lblServiceStatus->setStyleSheet("color: #666666; font-size: 11px;");
    }
}

void EnvironmentServiceWidget::updateModelStatus()
{
    if (!m_dlService || !m_dlService->isRunning()) {
        m_lblModelStatus->setText("📦 模型状态: 服务未启动");
        m_lblModelStatus->setStyleSheet("color: #888888; font-size: 11px;");
        return;
    }

    if (m_currentModelPath.isEmpty()) {
        m_lblModelStatus->setText("📦 模型状态: 未选择");
        m_lblModelStatus->setStyleSheet("color: #888888; font-size: 11px;");
        return;
    }

    // 检查当前选择的模型是否已加载
    if (m_dlService->isModelLoaded() && m_currentModelPath == m_loadedModelPath) {
        QString modelName = QFileInfo(m_currentModelPath).fileName();
        m_lblModelStatus->setText(QString("● 模型: %1").arg(modelName));
        m_lblModelStatus->setStyleSheet("color: #0066cc; font-size: 11px; font-weight: bold;");
    } else {
        m_lblModelStatus->setText("○ 模型状态: 未加载");
        m_lblModelStatus->setStyleSheet("color: #666666; font-size: 11px;");
    }
}

void EnvironmentServiceWidget::updateRunButtonState(bool enabled)
{
    Q_UNUSED(enabled);
}

void EnvironmentServiceWidget::scanEnvironments()
{
    emit logMessage("[环境服务] 开始扫描 Python 环境...");
    m_envCombo->clear();
    m_lblServiceStatus->setText("🔍 正在扫描环境...");
    m_lblServiceStatus->setStyleSheet("color: #0066cc; font-size: 11px;");

    // 初始化缓存管理器
    Utils::EnvironmentCacheManager *cacheMgr = Utils::EnvironmentCacheManager::instance();
    emit logMessage("[环境服务] 初始化环境缓存管理器...");
    cacheMgr->initialize();

    // 首先尝试从缓存获取环境列表（快速）
    QVector<Utils::CachedEnvironment> cachedEnvs = cacheMgr->getCachedEnvironments();
    emit logMessage(QString("[环境服务] 从缓存加载了 %1 个环境").arg(cachedEnvs.size()));

    if (!cachedEnvs.isEmpty()) {
        // 使用缓存数据快速填充界面
        populateEnvironmentCombo(cachedEnvs);

        // 启动后台验证（异步更新缓存）
        cacheMgr->startBackgroundValidation();

        m_lblServiceStatus->setText(QString("✓ 已加载 %1 个环境（来自缓存）").arg(cachedEnvs.size()));
        m_lblServiceStatus->setStyleSheet("color: #009900; font-size: 11px;");

        // 延迟执行完整扫描（更新缓存）- 使用扫描线程在后台更新，不阻塞UI
        emit logMessage("[环境服务] 启动后台环境扫描...");
        // 注意：扫描线程的信号已连接到 onScanCompleted，会自动更新UI
        // 这里我们不启动新扫描，让 cacheMgr 的后台验证来处理
        // 如果用户需要强制刷新，可以点击重载按钮

        return;
    }

    // 缓存为空，执行完整扫描
    emit logMessage("[环境服务] 缓存为空，执行完整环境扫描...");
    QVector<Utils::PythonEnvironment> envs = Utils::DLService::scanEnvironments();
    emit logMessage(QString("[环境服务] 扫描完成，找到 %1 个环境").arg(envs.size()));

    // 转换为 CachedEnvironment
    QVector<Utils::CachedEnvironment> cachedList;
    for (const auto &env : envs) {
        cachedList.append(Utils::CachedEnvironment(env));
    }

    populateEnvironmentCombo(cachedList);
}

void EnvironmentServiceWidget::populateEnvironmentCombo(const QVector<Utils::CachedEnvironment> &envs)
{
    emit logMessage(QString("[环境服务] 填充环境列表，共 %1 个环境").arg(envs.size()));

    // 获取缓存管理器
    Utils::EnvironmentCacheManager *cacheMgr = Utils::EnvironmentCacheManager::instance();

    // 获取上次使用的环境
    QString lastUsedEnv = cacheMgr->getLastUsedEnvironment();
    if (lastUsedEnv.isEmpty() && m_dlService) {
        lastUsedEnv = m_dlService->getLastUsedEnvironment();
    }
    emit logMessage(QString("[环境服务] 上次使用的环境: %1").arg(lastUsedEnv.isEmpty() ? "无" : lastUsedEnv));

    int selectedIndex = -1;
    int firstReadyIndex = -1;

    for (const auto &env : envs) {
        QString displayText = env.name;
        if (env.hasUltralytics) {
            displayText += " ✓";
        }
        displayText += QString(" (%1)").arg(env.type);

        // 添加版本信息提示
        QString tooltip = QString("路径: %1\nPython: %2").arg(env.path, env.pythonVersion);
        if (env.hasUltralytics) {
            tooltip += QString("\nUltralytics: %1").arg(env.ultralyticsVersion);
        }
        if (env.hasTorch) {
            tooltip += QString("\nPyTorch: %1").arg(env.torchVersion);
        }

        int index = m_envCombo->count();
        m_envCombo->addItem(displayText, env.path);
        m_envCombo->setItemData(index, tooltip, Qt::ToolTipRole);

        // 设置有 ultralytics 的项的背景色
        if (env.hasUltralytics) {
            m_envCombo->setItemData(index, QColor(200, 240, 200), Qt::BackgroundRole);
            if (firstReadyIndex < 0) {
                firstReadyIndex = index;
                emit logMessage(QString("[环境服务] 找到就绪环境: %1").arg(env.name));
            }
        }

        // 检查是否匹配上次使用的环境
        if (!lastUsedEnv.isEmpty() &&
            QDir::cleanPath(env.path) == QDir::cleanPath(lastUsedEnv)) {
            selectedIndex = index;
            emit logMessage(QString("[环境服务] 匹配到上次使用的环境: %1").arg(env.name));
        }
    }

    // 选择优先级：1. 上次使用的环境 2. 第一个就绪的环境 3. 第一个环境
    int indexToSelect = -1;
    if (selectedIndex >= 0) {
        indexToSelect = selectedIndex;
        emit logMessage("[环境服务] 自动选择上次使用的环境");
    } else if (firstReadyIndex >= 0) {
        indexToSelect = firstReadyIndex;
        emit logMessage("[环境服务] 自动选择第一个就绪的环境");
    } else if (!envs.isEmpty()) {
        indexToSelect = 0;
        emit logMessage("[环境服务] 自动选择第一个环境");
    } else {
        m_lblServiceStatus->setText("⚠ 未找到 Python 环境，请检查安装");
        m_lblServiceStatus->setStyleSheet("color: #cc3300; font-size: 11px;");
        emit logMessage("[环境服务] 错误: 未找到 Python 环境");
        return;
    }

    // 设置下拉框选择（这会触发 onEnvironmentChanged，但为了保险起见，我们也在这里设置环境路径）
    m_envCombo->setCurrentIndex(indexToSelect);

    // 确保环境路径被设置（防止信号未触发的情况）
    if (m_dlService && indexToSelect >= 0 && indexToSelect < envs.size()) {
        QString envPath = envs[indexToSelect].path;
        emit logMessage(QString("[环境服务] 预设环境路径: %1").arg(envPath));
        m_dlService->setEnvironmentPath(envPath);
        m_savedEnvPath = envPath;
    }
}

void EnvironmentServiceWidget::tryAutoStartService()
{
    emit logMessage("[环境服务] 尝试自动启动服务...");

    if (!m_dlService) {
        emit logMessage("[环境服务] 错误: DL 服务未初始化");
        return;
    }

    // 如果服务已运行，只尝试加载模型
    if (m_dlService->isRunning()) {
        emit logMessage("[环境服务] 服务已在运行中，尝试加载模型");
        tryAutoLoadModel();
        return;
    }

    // 检查当前选择的环境
    int index = m_envCombo->currentIndex();
    if (index < 0) {
        emit logMessage("[环境服务] 错误: 未选择环境");
        return;
    }

    QString envText = m_envCombo->itemText(index);
    QString envPath = m_envCombo->itemData(index).toString();

    emit logMessage(QString("[环境服务] 选中环境: %1").arg(envText));
    emit logMessage(QString("[环境服务] 环境路径: %1").arg(envPath));

    // 检查环境是否就绪（缓存中是否有 ultralytics）
    if (!envText.contains("✓")) {
        emit logMessage("[环境服务] 警告: 当前环境可能缺少 ultralytics，尝试启动...");
        // 不再直接返回，而是尝试启动（可能缓存过期，让启动过程去验证）
    }

    // 检查是否可以使用快速启动
    bool canFastStart = m_dlService->canFastStart(envPath);
    emit logMessage(QString("[环境服务] 快速启动可用: %1").arg(canFastStart ? "是" : "否"));

    if (canFastStart) {
        m_lblServiceStatus->setText("⏳ 正在快速启动服务...");
        m_lblServiceStatus->setStyleSheet("color: #0066cc; font-size: 11px;");
        emit logMessage("[环境服务] 使用快速启动模式...");

        // 使用快速启动（跳过验证）
        bool success = m_dlService->fastStart();

        if (success) {
            emit logMessage("[环境服务] 快速启动成功");
            onServiceStartedSuccessfully();
            return;
        }
        // 快速启动失败，回退到正常启动
        emit logMessage("[环境服务] 快速启动失败，回退到正常启动...");
        m_lblServiceStatus->setText("⏳ 快速启动失败，尝试正常启动...");
    } else {
        m_lblServiceStatus->setText("⏳ 正在启动服务...");
        m_lblServiceStatus->setStyleSheet("color: #0066cc; font-size: 11px;");
        emit logMessage("[环境服务] 使用正常启动模式...");
    }

    // 确保环境路径已设置（关键：在启动前必须设置环境路径）
    if (m_dlService->currentEnvironmentPath() != envPath) {
        emit logMessage(QString("[环境服务] 设置环境路径: %1").arg(envPath));
        m_dlService->setEnvironmentPath(envPath);
    }

    // 正常启动
    emit logMessage(QString("[环境服务] 正在启动 Python DL服务: %1").arg(envPath));
    bool success = m_dlService->start();

    if (success) {
        emit logMessage("[环境服务] 服务启动成功");
        onServiceStartedSuccessfully();
    } else {
        emit logMessage("[环境服务] 错误: 服务启动失败");
        m_lblServiceStatus->setText("✗ 服务启动失败");
        m_lblServiceStatus->setStyleSheet("color: #cc3300; font-size: 11px;");
    }
}

void EnvironmentServiceWidget::onServiceStartedSuccessfully()
{
    m_lblServiceStatus->setText("● 服务状态: 运行中");
    m_lblServiceStatus->setStyleSheet("color: #0066cc; font-size: 11px; font-weight: bold;");

    emit logMessage("[环境服务] DL 服务已成功启动并运行");
    emit serviceStarted();

    // 尝试自动加载模型
    emit logMessage("[环境服务] 准备自动加载模型...");
    QTimer::singleShot(100, this, &EnvironmentServiceWidget::tryAutoLoadModel);
}

void EnvironmentServiceWidget::tryAutoLoadModel()
{
    emit logMessage("[环境服务] 尝试自动加载模型...");

    if (!m_dlService) {
        emit logMessage("[环境服务] 错误: DL 服务未初始化，无法加载模型");
        updateModelStatus();
        return;
    }

    if (!m_dlService->isRunning()) {
        emit logMessage("[环境服务] 错误: 服务未运行，无法加载模型");
        updateModelStatus();
        return;
    }

    // 如果没有模型路径，尝试自动搜索对应任务的模型
    if (m_currentModelPath.isEmpty()) {
        emit logMessage("[环境服务] 未选择模型，尝试自动搜索...");
        QString autoModelPath = findFirstAvailableModel(m_currentTask);
        if (!autoModelPath.isEmpty()) {
            m_currentModelPath = autoModelPath;
            // 更新下拉框选择
            QString modelName = QFileInfo(autoModelPath).fileName();
            int index = m_cmbModelSelect->findData(autoModelPath);
            if (index >= 0) {
                m_cmbModelSelect->setCurrentIndex(index);
            } else {
                // 如果不在列表中，添加它
                m_cmbModelSelect->addItem(modelName, autoModelPath);
                m_cmbModelSelect->setCurrentIndex(m_cmbModelSelect->count() - 1);
            }
            emit logMessage(QString("[环境服务] 自动选择模型: %1").arg(modelName));
        } else {
            emit logMessage("[环境服务] 警告: 未找到可用模型，跳过加载");
            updateModelStatus();
            return;
        }
    }

    emit logMessage(QString("[环境服务] 当前模型路径: %1").arg(m_currentModelPath));

    // 如果当前模型已经加载，不重复加载
    if (!m_loadedModelPath.isEmpty() && m_currentModelPath == m_loadedModelPath && m_dlService->isModelLoaded()) {
        emit logMessage("[环境服务] 模型已加载，跳过重复加载");
        updateModelStatus();
        return;
    }

    m_lblModelStatus->setText("⏳ 正在加载模型...");
    m_lblModelStatus->setStyleSheet("color: #0066cc; font-size: 11px;");
    emit logMessage(QString("[环境服务] 正在加载模型: %1").arg(QFileInfo(m_currentModelPath).fileName()));

    bool success = m_dlService->loadModel(m_currentModelPath);

    if (success) {
        m_loadedModelPath = m_currentModelPath;  // 记录已加载的模型路径
        updateModelStatus();
        emit logMessage(QString("[环境服务] 模型加载成功: %1").arg(QFileInfo(m_currentModelPath).fileName()));
        emit modelLoaded(m_currentModelPath);
    } else {
        m_loadedModelPath.clear();  // 加载失败，清除已加载路径
        m_lblModelStatus->setText("✗ 模型状态: 加载失败");
        m_lblModelStatus->setStyleSheet("color: #cc3300; font-size: 11px;");
        emit logMessage("[环境服务] 错误: 模型加载失败");
    }
}

void EnvironmentServiceWidget::onEnvironmentChanged(int index)
{
    emit logMessage(QString("[环境服务] 环境选择变化，索引: %1").arg(index));

    if (index < 0) {
        emit logMessage("[环境服务] 无效的环境索引，取消操作");
        return;
    }

    QString envPath = m_envCombo->itemData(index).toString();
    QString envName = m_envCombo->itemText(index);

    emit logMessage(QString("[环境服务] 用户选择环境: %1").arg(envName));
    emit logMessage(QString("[环境服务] 环境路径: %1").arg(envPath));

    if (!m_dlService) {
        emit logMessage("[环境服务] 错误: DL 服务未初始化");
        return;
    }

    // 如果服务正在运行，先停止
    if (m_dlService->isRunning()) {
        emit logMessage("[环境服务] 正在停止当前运行的服务...");
        m_dlService->stop();
        m_loadedModelPath.clear();  // 清除已加载的模型路径
        emit serviceStopped();
        emit logMessage("[环境服务] 服务已停止");
    }

    // 设置新环境
    m_dlService->setEnvironmentPath(envPath);
    m_savedEnvPath = envPath;
    emit logMessage(QString("[环境服务] 环境已设置为: %1").arg(envName));
    emit environmentChanged(envPath);

    // 更新模型状态
    updateModelStatus();

    // 自动启动服务（使用较短的延迟确保服务已停止）
    emit logMessage("[环境服务] 准备启动新环境服务...");
    QTimer::singleShot(200, this, [this]() {
        emit logMessage("[环境服务] 触发自动启动...");
        tryAutoStartService();
    });
}

void EnvironmentServiceWidget::onBrowseModel()
{
    QString modelPath = QFileDialog::getOpenFileName(
        this,
        "选择 DL 模型文件",
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

void EnvironmentServiceWidget::onReloadEnvironments()
{
    emit logMessage("[环境服务] ================================");
    emit logMessage("[环境服务] 用户点击重载环境按钮");
    emit logMessage("[环境服务] ================================");

    // 检查是否已经在扫描中
    if (m_envScanner->isScanning()) {
        emit logMessage("[环境服务] 警告: 扫描已在进行中，请等待...");
        return;
    }

    // 禁用按钮防止重复点击
    m_btnReloadEnvs->setEnabled(false);
    m_btnReloadEnvs->setText("⏳");

    // 停止当前运行的服务
    if (m_dlService && m_dlService->isRunning()) {
        emit logMessage("[环境服务] 停止当前运行的服务...");
        m_dlService->stop();
        m_loadedModelPath.clear();
        emit serviceStopped();
        emit logMessage("[环境服务] 服务已停止");
    }

    // 清空缓存
    emit logMessage("[环境服务] 正在清空环境缓存...");
    Utils::EnvironmentCacheManager *cacheMgr = Utils::EnvironmentCacheManager::instance();
    cacheMgr->clearCache();
    emit logMessage("[环境服务] 缓存已清空");

    // 清空下拉框
    m_envCombo->clear();
    m_lblServiceStatus->setText("🔄 正在重新检测环境...");
    m_lblServiceStatus->setStyleSheet("color: #0066cc; font-size: 11px;");

    emit logMessage("[环境服务] 开始在后台线程扫描环境...");

    // 在后台线程执行扫描（使用 QtConcurrent::run）
    // 注意：扫描完成后会通过 scanCompleted 信号自动调用 onScanCompleted
    QFuture<void> future = QtConcurrent::run([this]() {
        // 执行同步扫描（信号会自动发射）
        m_envScanner->scan();
    });
}

void EnvironmentServiceWidget::onScanProgress(const QString &message)
{
    emit logMessage(QString("[环境扫描] %1").arg(message));
}

void EnvironmentServiceWidget::onScanCompleted(const QVector<Utils::CachedEnvironment> &environments)
{
    emit logMessage(QString("[环境服务] 扫描完成，找到 %1 个环境").arg(environments.size()));

    // 保存扫描结果
    m_scannedEnvironments = environments;

    // 将扫描结果存入缓存管理器
    Utils::EnvironmentCacheManager *cacheMgr = Utils::EnvironmentCacheManager::instance();
    for (const auto &env : environments) {
        cacheMgr->updateCache(env);
    }

    // 填充下拉框
    populateEnvironmentCombo(m_scannedEnvironments);

    // 更新状态标签（扫描时未验证 ultralytics，所以就绪数可能为0）
    int readyCount = 0;
    for (const auto &env : m_scannedEnvironments) {
        if (env.hasUltralytics) readyCount++;
    }
    m_lblServiceStatus->setText(QString("✓ 已重新加载 %1 个环境 (%2 个就绪)").arg(environments.size()).arg(readyCount));
    m_lblServiceStatus->setStyleSheet("color: #009900; font-size: 11px;");

    // 恢复按钮
    m_btnReloadEnvs->setEnabled(true);
    m_btnReloadEnvs->setText("🔄");

    emit logMessage("[环境服务] ================================");
    emit logMessage("[环境服务] 环境重载完成");
    emit logMessage(QString("[环境服务] 总计: %1 个环境, 就绪: %2 个").arg(environments.size()).arg(readyCount));
    emit logMessage("[环境服务] ================================");

    // 启动后台验证来检查 ultralytics（更新就绪状态）
    if (!environments.isEmpty()) {
        emit logMessage("[环境服务] 启动后台验证检查 ultralytics...");
        cacheMgr->startBackgroundValidation();
    }

    // 尝试自动启动（即使没有验证通过，让启动过程去验证）
    emit logMessage("[环境服务] 准备尝试自动启动...");
    QTimer::singleShot(500, this, &EnvironmentServiceWidget::tryAutoStartService);
}

void EnvironmentServiceWidget::onScanFailed(const QString &errorMessage)
{
    emit logMessage(QString("[环境服务] 错误: %1").arg(errorMessage));

    // 恢复按钮
    m_btnReloadEnvs->setEnabled(true);
    m_btnReloadEnvs->setText("🔄");

    m_lblServiceStatus->setText("✗ 扫描失败: " + errorMessage);
    m_lblServiceStatus->setStyleSheet("color: #cc3300; font-size: 11px;");
}

void EnvironmentServiceWidget::onModelSelectionChanged(int index)
{
    emit logMessage(QString("[环境服务] 模型选择变化，索引: %1").arg(index));

    if (index < 0) {
        emit logMessage("[环境服务] 无效的模型索引，取消操作");
        return;
    }

    QString modelPath = m_cmbModelSelect->itemData(index).toString();
    QString modelName = m_cmbModelSelect->itemText(index);
    m_currentModelPath = modelPath;

    emit logMessage(QString("[环境服务] 用户选择模型: %1").arg(modelName));
    emit logMessage(QString("[环境服务] 模型路径: %1").arg(modelPath));

    // 更新模型状态
    updateModelStatus();

    // 尝试自动加载模型
    emit logMessage("[环境服务] 准备加载新选择的模型...");
    QTimer::singleShot(100, this, [this]() {
        emit logMessage("[环境服务] 触发模型加载...");
        tryAutoLoadModel();
    });
}

void EnvironmentServiceWidget::updateModelList(Models::CVTask task)
{
    m_currentTask = task;

    // 清空当前模型路径，避免旧模型路径残留
    m_currentModelPath.clear();

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
        m_lblModelStatus->setText("📁 模型状态: 目录不存在");
        m_lblModelStatus->setStyleSheet("color: #cc3300; font-size: 11px;");
        emit logMessage(QString("模型目录不存在: %1").arg(modelDir));
        return;
    }

    // 支持的模型文件扩展名（仅 PyTorch 格式）
    QStringList filters;
    filters << "*.pt" << "*.pth";

    QFileInfoList fileList = dirObj.entryInfoList(filters, QDir::Files | QDir::Readable);

    if (fileList.isEmpty()) {
        m_cmbModelSelect->setPlaceholderText("未找到模型文件");
        m_lblModelStatus->setText("📦 模型状态: 无可用模型");
        m_lblModelStatus->setStyleSheet("color: #888888; font-size: 11px;");
        emit logMessage(QString("未在 %1 目录找到模型文件").arg(modelDir));
    } else {
        // 注意：切换任务时不恢复上次使用的模型（避免跨任务混淆）
        // 只使用当前任务目录下的模型

        // 添加所有模型到列表
        for (int i = 0; i < fileList.size(); ++i) {
            const QFileInfo &fileInfo = fileList[i];
            m_cmbModelSelect->addItem(fileInfo.fileName(), fileInfo.absoluteFilePath());
        }

        // 自动选择第一个模型
        if (!fileList.isEmpty()) {
            m_cmbModelSelect->setCurrentIndex(0);
        }

        // 同步更新当前模型路径（确保自动选择后能正确加载）
        int currentIndex = m_cmbModelSelect->currentIndex();
        if (currentIndex >= 0) {
            m_currentModelPath = m_cmbModelSelect->itemData(currentIndex).toString();
            emit logMessage(QString("[环境服务] 自动选择模型: %1").arg(QFileInfo(m_currentModelPath).fileName()));
        }

        m_lblModelStatus->setText(QString("✓ 已找到 %1 个模型").arg(fileList.size()));
        m_lblModelStatus->setStyleSheet("color: #ffffff; font-size: 11px;");
        emit logMessage(QString("已扫描到 %1 个可用模型").arg(fileList.size()));

        // 如果服务已在运行，尝试自动加载新任务的模型
        if (m_dlService && m_dlService->isRunning() && !m_currentModelPath.isEmpty()) {
            emit logMessage("[环境服务] 任务切换，尝试加载新模型...");
            QTimer::singleShot(100, this, &EnvironmentServiceWidget::tryAutoLoadModel);
        }
    }
}

QString EnvironmentServiceWidget::findFirstAvailableModel(Models::CVTask task) const
{
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
        return QString();
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
        return QString();
    }

    // 支持的模型文件扩展名（仅 PyTorch 格式）
    QStringList filters;
    filters << "*.pt" << "*.pth";

    QFileInfoList fileList = dirObj.entryInfoList(filters, QDir::Files | QDir::Readable);

    if (fileList.isEmpty()) {
        return QString();
    }

    // 返回第一个可用模型（优先 .pt 文件）
    // 先找 .pt 或 .pth 文件（PyTorch 格式）
    for (const QFileInfo &fileInfo : fileList) {
        QString suffix = fileInfo.suffix().toLower();
        if (suffix == "pt" || suffix == "pth") {
            return fileInfo.absoluteFilePath();
        }
    }

    // 如果没有 PyTorch 格式，返回第一个可用文件
    return fileList.first().absoluteFilePath();
}

} // namespace Widgets
} // namespace GenPreCVSystem
