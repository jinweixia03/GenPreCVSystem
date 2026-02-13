#include "settingsdialog.h"
#include "../utils/appsettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QDialogButtonBox>

namespace GenPreCVSystem {
namespace Views {

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_editOpenDir(nullptr)
    , m_editExportDir(nullptr)
    , m_spinMaxRecentFiles(nullptr)
    , m_chkAutoSave(nullptr)
    , m_comboExportFormat(nullptr)
    , m_chkIncludeTimestamp(nullptr)
    , m_chkIncludeMetadata(nullptr)
{
    setupUI();
    applyStyles();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    setWindowTitle(tr("设置"));
    setMinimumSize(450, 400);
    resize(500, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // ========== 目录设置组 ==========
    QGroupBox *dirGroup = new QGroupBox(tr("目录设置"), this);
    QFormLayout *dirLayout = new QFormLayout(dirGroup);
    dirLayout->setSpacing(10);

    // 默认打开目录
    QWidget *openDirWidget = new QWidget();
    QHBoxLayout *openDirLayout = new QHBoxLayout(openDirWidget);
    openDirLayout->setContentsMargins(0, 0, 0, 0);
    m_editOpenDir = new QLineEdit();
    m_editOpenDir->setPlaceholderText(tr("选择默认打开目录"));
    QPushButton *btnBrowseOpen = new QPushButton(tr("浏览..."));
    btnBrowseOpen->setFixedWidth(70);
    connect(btnBrowseOpen, &QPushButton::clicked, this, &SettingsDialog::onBrowseOpenDirectory);
    openDirLayout->addWidget(m_editOpenDir);
    openDirLayout->addWidget(btnBrowseOpen);
    dirLayout->addRow(tr("默认打开目录:"), openDirWidget);

    // 默认导出目录
    QWidget *exportDirWidget = new QWidget();
    QHBoxLayout *exportDirLayout = new QHBoxLayout(exportDirWidget);
    exportDirLayout->setContentsMargins(0, 0, 0, 0);
    m_editExportDir = new QLineEdit();
    m_editExportDir->setPlaceholderText(tr("选择默认导出目录"));
    QPushButton *btnBrowseExport = new QPushButton(tr("浏览..."));
    btnBrowseExport->setFixedWidth(70);
    connect(btnBrowseExport, &QPushButton::clicked, this, &SettingsDialog::onBrowseExportDirectory);
    exportDirLayout->addWidget(m_editExportDir);
    exportDirLayout->addWidget(btnBrowseExport);
    dirLayout->addRow(tr("默认导出目录:"), exportDirWidget);

    mainLayout->addWidget(dirGroup);

    // ========== 常规设置组 ==========
    QGroupBox *generalGroup = new QGroupBox(tr("常规设置"), this);
    QFormLayout *generalLayout = new QFormLayout(generalGroup);
    generalLayout->setSpacing(10);

    // 最近文件数量
    m_spinMaxRecentFiles = new QSpinBox();
    m_spinMaxRecentFiles->setRange(1, 20);
    m_spinMaxRecentFiles->setValue(10);
    generalLayout->addRow(tr("最近文件数量:"), m_spinMaxRecentFiles);

    // 自动保存结果
    m_chkAutoSave = new QCheckBox(tr("自动保存检测结果"));
    generalLayout->addRow(QString(), m_chkAutoSave);

    mainLayout->addWidget(generalGroup);

    // ========== 导出设置组 ==========
    QGroupBox *exportGroup = new QGroupBox(tr("导出设置"), this);
    QFormLayout *exportLayout = new QFormLayout(exportGroup);
    exportLayout->setSpacing(10);

    // 导出格式
    m_comboExportFormat = new QComboBox();
    m_comboExportFormat->addItem(tr("JSON"), "JSON");
    m_comboExportFormat->addItem(tr("CSV"), "CSV");
    exportLayout->addRow(tr("导出格式:"), m_comboExportFormat);

    // 包含时间戳
    m_chkIncludeTimestamp = new QCheckBox(tr("导出时包含时间戳"));
    exportLayout->addRow(QString(), m_chkIncludeTimestamp);

    // 包含元数据
    m_chkIncludeMetadata = new QCheckBox(tr("导出时包含元数据"));
    exportLayout->addRow(QString(), m_chkIncludeMetadata);

    mainLayout->addWidget(exportGroup);

    mainLayout->addStretch();

    // ========== 按钮区域 ==========
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Reset | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定"));
    buttonBox->button(QDialogButtonBox::Apply)->setText(tr("应用"));
    buttonBox->button(QDialogButtonBox::Reset)->setText(tr("恢复默认"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);

    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::applyStyles()
{
    setStyleSheet(
        "QDialog { background-color: #1e1e1e; color: #cccccc; }"
        "QLabel { color: #cccccc; }"
        "QGroupBox { "
        "  color: #cccccc; "
        "  font-weight: bold; "
        "  border: 1px solid #3e3e42; "
        "  border-radius: 4px; "
        "  margin-top: 10px; "
        "  padding-top: 10px; "
        "} "
        "QGroupBox::title { "
        "  subcontrol-origin: margin; "
        "  left: 10px; "
        "  padding: 0 5px; "
        "} "
        "QLineEdit { "
        "  background-color: #3c3c3c; "
        "  color: #cccccc; "
        "  border: 1px solid #3e3e42; "
        "  border-radius: 2px; "
        "  padding: 5px; "
        "} "
        "QLineEdit:focus { border: 1px solid #0078d4; } "
        "QSpinBox { "
        "  background-color: #3c3c3c; "
        "  color: #cccccc; "
        "  border: 1px solid #3e3e42; "
        "  border-radius: 2px; "
        "  padding: 3px; "
        "} "
        "QSpinBox:focus { border: 1px solid #0078d4; } "
        "QComboBox { "
        "  background-color: #3c3c3c; "
        "  color: #cccccc; "
        "  border: 1px solid #3e3e42; "
        "  border-radius: 2px; "
        "  padding: 5px; "
        "} "
        "QComboBox:focus { border: 1px solid #0078d4; } "
        "QComboBox::drop-down { border: none; width: 20px; } "
        "QComboBox QAbstractItemView { "
        "  background-color: #3c3c3c; "
        "  color: #cccccc; "
        "  selection-background-color: #0078d4; "
        "} "
        "QCheckBox { color: #cccccc; } "
        "QCheckBox::indicator { width: 16px; height: 16px; } "
        "QPushButton { "
        "  background-color: #0e639c; "
        "  color: #ffffff; "
        "  border: none; "
        "  padding: 6px 16px; "
        "  border-radius: 2px; "
        "} "
        "QPushButton:hover { background-color: #1177bb; } "
        "QPushButton:pressed { background-color: #0e639c; } "
        "QDialogButtonBox QPushButton { min-width: 70px; }"
    );
}

void SettingsDialog::loadSettings()
{
    m_editOpenDir->setText(Utils::AppSettings::defaultOpenDirectory());
    m_editExportDir->setText(Utils::AppSettings::defaultExportDirectory());
    m_spinMaxRecentFiles->setValue(Utils::AppSettings::maxRecentFiles());
    m_chkAutoSave->setChecked(Utils::AppSettings::autoSaveResults());

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
    Utils::AppSettings::setDefaultOpenDirectory(m_editOpenDir->text());
    Utils::AppSettings::setDefaultExportDirectory(m_editExportDir->text());
    Utils::AppSettings::setMaxRecentFiles(m_spinMaxRecentFiles->value());
    Utils::AppSettings::setAutoSaveResults(m_chkAutoSave->isChecked());
    Utils::AppSettings::setExportFormat(m_comboExportFormat->currentData().toString());
    Utils::AppSettings::setIncludeTimestamp(m_chkIncludeTimestamp->isChecked());
    Utils::AppSettings::setIncludeMetadata(m_chkIncludeMetadata->isChecked());
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
