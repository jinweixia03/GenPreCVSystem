#include "settingsdialog.h"
#include "../utils/appsettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QTabWidget>

namespace GenPreCVSystem {
namespace Views {

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    loadSettings();
    applyStyles();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    setWindowTitle(tr("设置"));
    setMinimumSize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 创建标签页
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(), tr("通用"));
    m_tabWidget->addTab(createYOLOTab(), tr("YOLO"));
    m_tabWidget->addTab(createExportTab(), tr("导出"));
    mainLayout->addWidget(m_tabWidget);

    // 创建按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_btnReset = new QPushButton(tr("恢复默认"), this);
    m_btnOK = new QPushButton(tr("确定"), this);
    m_btnCancel = new QPushButton(tr("取消"), this);
    m_btnApply = new QPushButton(tr("应用"), this);

    m_btnOK->setDefault(true);

    buttonLayout->addWidget(m_btnReset);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(m_btnOK);
    buttonLayout->addWidget(m_btnCancel);
    buttonLayout->addWidget(m_btnApply);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(m_btnOK, &QPushButton::clicked, this, &SettingsDialog::onAccept);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnApply, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_btnReset, &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);
}

QWidget* SettingsDialog::createGeneralTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(15);

    // 默认目录组
    QGroupBox *dirGroup = new QGroupBox(tr("默认目录"), tab);
    QFormLayout *dirLayout = new QFormLayout(dirGroup);

    // 打开目录
    QWidget *openDirWidget = new QWidget();
    QHBoxLayout *openDirLayout = new QHBoxLayout(openDirWidget);
    openDirLayout->setContentsMargins(0, 0, 0, 0);
    m_editOpenDir = new QLineEdit();
    QPushButton *btnBrowseOpen = new QPushButton(tr("浏览..."));
    openDirLayout->addWidget(m_editOpenDir);
    openDirLayout->addWidget(btnBrowseOpen);
    dirLayout->addRow(tr("打开目录:"), openDirWidget);
    connect(btnBrowseOpen, &QPushButton::clicked, this, &SettingsDialog::onBrowseOpenDirectory);

    // 导出目录
    QWidget *exportDirWidget = new QWidget();
    QHBoxLayout *exportDirLayout = new QHBoxLayout(exportDirWidget);
    exportDirLayout->setContentsMargins(0, 0, 0, 0);
    m_editExportDir = new QLineEdit();
    QPushButton *btnBrowseExport = new QPushButton(tr("浏览..."));
    exportDirLayout->addWidget(m_editExportDir);
    exportDirLayout->addWidget(btnBrowseExport);
    dirLayout->addRow(tr("导出目录:"), exportDirWidget);
    connect(btnBrowseExport, &QPushButton::clicked, this, &SettingsDialog::onBrowseExportDirectory);

    layout->addWidget(dirGroup);

    // 其他设置组
    QGroupBox *otherGroup = new QGroupBox(tr("其他设置"), tab);
    QFormLayout *otherLayout = new QFormLayout(otherGroup);

    m_spinMaxRecent = new QSpinBox();
    m_spinMaxRecent->setRange(1, 20);
    m_spinMaxRecent->setValue(10);
    otherLayout->addRow(tr("最近文件数量:"), m_spinMaxRecent);

    m_chkAutoSave = new QCheckBox(tr("自动保存检测结果"));
    otherLayout->addRow("", m_chkAutoSave);

    layout->addWidget(otherGroup);
    layout->addStretch();

    return tab;
}

QWidget* SettingsDialog::createYOLOTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(15);

    // Python 环境组
    QGroupBox *pythonGroup = new QGroupBox(tr("Python 环境"), tab);
    QFormLayout *pythonLayout = new QFormLayout(pythonGroup);

    QWidget *pythonWidget = new QWidget();
    QHBoxLayout *pythonHLayout = new QHBoxLayout(pythonWidget);
    pythonHLayout->setContentsMargins(0, 0, 0, 0);
    m_editPythonPath = new QLineEdit();
    m_btnBrowsePython = new QPushButton(tr("浏览..."));
    pythonHLayout->addWidget(m_editPythonPath);
    pythonHLayout->addWidget(m_btnBrowsePython);
    pythonLayout->addRow(tr("Python 路径:"), pythonWidget);

    connect(m_btnBrowsePython, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("选择 Python 可执行文件"),
                                                     m_editPythonPath->text(),
                                                     "Python (python.exe python)");
        if (!path.isEmpty()) {
            m_editPythonPath->setText(path);
        }
    });

    layout->addWidget(pythonGroup);

    // 检测参数组
    QGroupBox *paramsGroup = new QGroupBox(tr("检测参数"), tab);
    QFormLayout *paramsLayout = new QFormLayout(paramsGroup);

    m_spinConfThreshold = new QDoubleSpinBox();
    m_spinConfThreshold->setRange(0.01, 1.0);
    m_spinConfThreshold->setSingleStep(0.05);
    m_spinConfThreshold->setDecimals(2);
    paramsLayout->addRow(tr("置信度阈值:"), m_spinConfThreshold);

    m_spinIOUThreshold = new QDoubleSpinBox();
    m_spinIOUThreshold->setRange(0.01, 1.0);
    m_spinIOUThreshold->setSingleStep(0.05);
    m_spinIOUThreshold->setDecimals(2);
    paramsLayout->addRow(tr("IOU 阈值:"), m_spinIOUThreshold);

    m_spinImageSize = new QSpinBox();
    m_spinImageSize->setRange(128, 2048);
    m_spinImageSize->setSingleStep(64);
    paramsLayout->addRow(tr("图像尺寸:"), m_spinImageSize);

    layout->addWidget(paramsGroup);
    layout->addStretch();

    return tab;
}

QWidget* SettingsDialog::createExportTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(15);

    // 导出格式组
    QGroupBox *formatGroup = new QGroupBox(tr("导出格式"), tab);
    QFormLayout *formatLayout = new QFormLayout(formatGroup);

    m_comboExportFormat = new QComboBox();
    m_comboExportFormat->addItem("JSON", "JSON");
    m_comboExportFormat->addItem("CSV", "CSV");
    formatLayout->addRow(tr("默认格式:"), m_comboExportFormat);

    layout->addWidget(formatGroup);

    // 导出选项组
    QGroupBox *optionsGroup = new QGroupBox(tr("导出选项"), tab);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);

    m_chkIncludeTimestamp = new QCheckBox(tr("在文件名中包含时间戳"));
    optionsLayout->addWidget(m_chkIncludeTimestamp);

    m_chkIncludeMetadata = new QCheckBox(tr("包含元数据（图像路径、模型信息等）"));
    optionsLayout->addWidget(m_chkIncludeMetadata);

    layout->addWidget(optionsGroup);
    layout->addStretch();

    return tab;
}

void SettingsDialog::loadSettings()
{
    // 通用设置
    m_editOpenDir->setText(Utils::AppSettings::defaultOpenDirectory());
    m_editExportDir->setText(Utils::AppSettings::defaultExportDirectory());
    m_spinMaxRecent->setValue(Utils::AppSettings::maxRecentFiles());
    m_chkAutoSave->setChecked(Utils::AppSettings::autoSaveResults());

    // YOLO 设置
    m_editPythonPath->setText(Utils::AppSettings::pythonEnvironment());
    m_spinConfThreshold->setValue(Utils::AppSettings::defaultConfThreshold());
    m_spinIOUThreshold->setValue(Utils::AppSettings::defaultIOUThreshold());
    m_spinImageSize->setValue(Utils::AppSettings::defaultImageSize());

    // 导出设置
    QString format = Utils::AppSettings::exportFormat();
    int index = m_comboExportFormat->findData(format);
    if (index >= 0) {
        m_comboExportFormat->setCurrentIndex(index);
    }
    m_chkIncludeTimestamp->setChecked(Utils::AppSettings::includeTimestamp());
    m_chkIncludeMetadata->setChecked(Utils::AppSettings::includeMetadata());
}

void SettingsDialog::saveSettings()
{
    // 通用设置
    Utils::AppSettings::setDefaultOpenDirectory(m_editOpenDir->text());
    Utils::AppSettings::setDefaultExportDirectory(m_editExportDir->text());
    Utils::AppSettings::setMaxRecentFiles(m_spinMaxRecent->value());
    Utils::AppSettings::setAutoSaveResults(m_chkAutoSave->isChecked());

    // YOLO 设置
    Utils::AppSettings::setPythonEnvironment(m_editPythonPath->text());
    Utils::AppSettings::setDefaultConfThreshold(static_cast<float>(m_spinConfThreshold->value()));
    Utils::AppSettings::setDefaultIOUThreshold(static_cast<float>(m_spinIOUThreshold->value()));
    Utils::AppSettings::setDefaultImageSize(m_spinImageSize->value());

    // 导出设置
    Utils::AppSettings::setExportFormat(m_comboExportFormat->currentData().toString());
    Utils::AppSettings::setIncludeTimestamp(m_chkIncludeTimestamp->isChecked());
    Utils::AppSettings::setIncludeMetadata(m_chkIncludeMetadata->isChecked());
}

void SettingsDialog::applyStyles()
{
    setStyleSheet(
        "QDialog { background-color: #1e1e1e; color: #cccccc; }"
        "QTabWidget::pane { border: 1px solid #3e3e42; background-color: #252526; }"
        "QTabBar::tab { background-color: #2d2d30; color: #cccccc; padding: 8px 16px; border: 1px solid #3e3e42; }"
        "QTabBar::tab:selected { background-color: #252526; border-bottom-color: #252526; }"
        "QTabBar::tab:hover { background-color: #3e3e42; }"
        "QGroupBox { border: 1px solid #3e3e42; border-radius: 4px; margin-top: 8px; padding-top: 8px; color: #cccccc; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
        "QLabel { color: #cccccc; }"
        "QLineEdit { background-color: #3c3c3c; color: #cccccc; border: 1px solid #3e3e42; padding: 5px; border-radius: 2px; }"
        "QLineEdit:focus { border-color: #0078d4; }"
        "QSpinBox, QDoubleSpinBox { background-color: #3c3c3c; color: #cccccc; border: 1px solid #3e3e42; padding: 3px; border-radius: 2px; }"
        "QComboBox { background-color: #3c3c3c; color: #cccccc; border: 1px solid #3e3e42; padding: 5px; border-radius: 2px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 5px solid #cccccc; }"
        "QCheckBox { color: #cccccc; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
        "QPushButton { background-color: #0e639c; color: #ffffff; border: none; padding: 8px 16px; border-radius: 2px; }"
        "QPushButton:hover { background-color: #1177bb; }"
        "QPushButton:pressed { background-color: #0e639c; }"
        "QPushButton#btnCancel { background-color: #3c3c3c; }"
        "QPushButton#btnCancel:hover { background-color: #4e4e4e; }"
    );

    m_btnCancel->setObjectName("btnCancel");
}

void SettingsDialog::onBrowseOpenDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择默认打开目录"),
                                                     m_editOpenDir->text());
    if (!dir.isEmpty()) {
        m_editOpenDir->setText(dir);
    }
}

void SettingsDialog::onBrowseExportDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择默认导出目录"),
                                                     m_editExportDir->text());
    if (!dir.isEmpty()) {
        m_editExportDir->setText(dir);
    }
}

void SettingsDialog::onAccept()
{
    saveSettings();
    emit settingsChanged();
    accept();
}

void SettingsDialog::onApply()
{
    saveSettings();
    emit settingsChanged();
}

void SettingsDialog::onResetDefaults()
{
    Utils::AppSettings::resetToDefaults();
    loadSettings();
}

} // namespace Views
} // namespace GenPreCVSystem
