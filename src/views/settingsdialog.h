#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QPushButton;

namespace GenPreCVSystem {
namespace Views {

/**
 * @brief 设置对话框
 *
 * 提供应用程序设置的三标签页界面：通用、YOLO、导出
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
    QWidget* createGeneralTab();
    QWidget* createYOLOTab();
    QWidget* createExportTab();
    void loadSettings();
    void saveSettings();
    void applyStyles();

    QTabWidget *m_tabWidget;

    // 通用标签页
    QLineEdit *m_editOpenDir;
    QLineEdit *m_editExportDir;
    QSpinBox *m_spinMaxRecent;
    QCheckBox *m_chkAutoSave;

    // YOLO 标签页
    QLineEdit *m_editPythonPath;
    QPushButton *m_btnBrowsePython;
    QDoubleSpinBox *m_spinConfThreshold;
    QDoubleSpinBox *m_spinIOUThreshold;
    QSpinBox *m_spinImageSize;

    // 导出标签页
    QComboBox *m_comboExportFormat;
    QCheckBox *m_chkIncludeTimestamp;
    QCheckBox *m_chkIncludeMetadata;

    // 按钮
    QPushButton *m_btnOK;
    QPushButton *m_btnCancel;
    QPushButton *m_btnApply;
    QPushButton *m_btnReset;
};

} // namespace Views
} // namespace GenPreCVSystem

#endif // SETTINGSDIALOG_H
