#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QComboBox;

namespace GenPreCVSystem {
namespace Views {

/**
 * @brief 设置对话框
 *
 * 提供常规设置选项
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

signals:
    /**
     * @brief 设置已更改信号
     */
    void settingsChanged();

private slots:
    void onBrowseOpenDirectory();
    void onBrowseExportDirectory();
    void onAccept();
    void onApply();
    void onResetDefaults();

private:
    void setupUI();
    void applyStyles();
    void loadSettings();
    void saveSettings();

    // 目录设置
    QLineEdit *m_editOpenDir;
    QLineEdit *m_editExportDir;

    // 常规设置
    QSpinBox *m_spinMaxRecentFiles;
    QCheckBox *m_chkAutoSave;

    // 导出设置
    QComboBox *m_comboExportFormat;
    QCheckBox *m_chkIncludeTimestamp;
    QCheckBox *m_chkIncludeMetadata;
};

} // namespace Views
} // namespace GenPreCVSystem

#endif // SETTINGSDIALOG_H
