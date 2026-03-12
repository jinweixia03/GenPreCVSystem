/**
 * @file settingsdialog.cpp
 * @brief 设置对话框实现
 *
 * 应用程序设置界面，包含：
 * - 目录设置（默认打开/导出目录）
 * - 常规设置（最近文件数量）
 */

#include "settingsdialog.h"
#include "appsettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
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
    setWindowTitle(tr("⚙ 设置"));
    setMinimumSize(450, 250);
    resize(500, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // ========== 目录设置组 ==========
    QGroupBox *dirGroup = new QGroupBox(tr("📁 目录设置"), this);
    QFormLayout *dirLayout = new QFormLayout(dirGroup);
    dirLayout->setSpacing(10);

    // 默认打开目录
    QWidget *openDirWidget = new QWidget();
    QHBoxLayout *openDirLayout = new QHBoxLayout(openDirWidget);
    openDirLayout->setContentsMargins(0, 0, 0, 0);
    m_editOpenDir = new QLineEdit();
    m_editOpenDir->setPlaceholderText(tr("选择默认打开目录"));
    QPushButton *btnBrowseOpen = new QPushButton(tr("📂 浏览..."));
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
    QPushButton *btnBrowseExport = new QPushButton(tr("📂 浏览..."));
    btnBrowseExport->setFixedWidth(70);
    connect(btnBrowseExport, &QPushButton::clicked, this, &SettingsDialog::onBrowseExportDirectory);
    exportDirLayout->addWidget(m_editExportDir);
    exportDirLayout->addWidget(btnBrowseExport);
    dirLayout->addRow(tr("默认导出目录:"), exportDirWidget);

    mainLayout->addWidget(dirGroup);

    // ========== 常规设置组 ==========
    QGroupBox *generalGroup = new QGroupBox(tr("⚙ 常规设置"), this);
    QFormLayout *generalLayout = new QFormLayout(generalGroup);
    generalLayout->setSpacing(10);

    // 最近文件数量
    m_spinMaxRecentFiles = new QSpinBox();
    m_spinMaxRecentFiles->setRange(1, 20);
    m_spinMaxRecentFiles->setValue(10);
    generalLayout->addRow(tr("最近文件数量:"), m_spinMaxRecentFiles);

    mainLayout->addWidget(generalGroup);

    mainLayout->addStretch();

    // ========== 按钮区域 ==========
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Reset | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("✓ 确定"));
    buttonBox->button(QDialogButtonBox::Apply)->setText(tr("▶ 应用"));
    buttonBox->button(QDialogButtonBox::Reset)->setText(tr("↺ 恢复默认"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("✕ 取消"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);

    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::applyStyles()
{
    setStyleSheet(
        "QDialog { background-color: #ffffff; color: #000000; }"
        "QLabel { color: #000000; }"
        "QGroupBox { "
        "  color: #000000; "
        "  font-weight: bold; "
        "  border: 1px solid #0066cc; "
        "  border-radius: 0px; "
        "  margin-top: 10px; "
        "  padding-top: 10px; "
        "} "
        "QGroupBox::title { "
        "  subcontrol-origin: margin; "
        "  left: 10px; "
        "  padding: 0 5px; "
        "  color: #0066cc; "
        "} "
        "QLineEdit { "
        "  background-color: #ffffff; "
        "  color: #000000; "
        "  border: 1px solid #c0c0c0; "
        "  border-radius: 0px; "
        "  padding: 5px; "
        "} "
        "QLineEdit:focus { border: 1px solid #0066cc; } "
        "QSpinBox { "
        "  background-color: #ffffff; "
        "  color: #000000; "
        "  border: 1px solid #c0c0c0; "
        "  border-radius: 0px; "
        "  padding: 3px; "
        "} "
        "QSpinBox:focus { border: 1px solid #0066cc; } "
        "QPushButton { "
        "  background-color: #0066cc; "
        "  color: #ffffff; "
        "  border: none; "
        "  padding: 6px 16px; "
        "  border-radius: 0px; "
        "} "
        "QPushButton:hover { background-color: #0077dd; } "
        "QPushButton:pressed { background-color: #0055aa; } "
        "QDialogButtonBox QPushButton { min-width: 70px; }"
    );
}

void SettingsDialog::loadSettings()
{
    m_editOpenDir->setText(Utils::AppSettings::defaultOpenDirectory());
    m_editExportDir->setText(Utils::AppSettings::defaultExportDirectory());
    m_spinMaxRecentFiles->setValue(Utils::AppSettings::maxRecentFiles());
}

void SettingsDialog::saveSettings()
{
    Utils::AppSettings::setDefaultOpenDirectory(m_editOpenDir->text());
    Utils::AppSettings::setDefaultExportDirectory(m_editExportDir->text());
    Utils::AppSettings::setMaxRecentFiles(m_spinMaxRecentFiles->value());
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
