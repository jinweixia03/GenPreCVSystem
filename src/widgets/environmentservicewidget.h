#ifndef ENVIRONMENTSERVICEWIDGET_H
#define ENVIRONMENTSERVICEWIDGET_H

#include <QWidget>
#include <QVector>
#include "../utils/yoloservice.h"
#include "../models/tasktypes.h"

class QComboBox;
class QPushButton;
class QLabel;

namespace GenPreCVSystem {
namespace Widgets {

/**
 * @brief 共享的环境和服务控制组件
 *
 * 此组件包含所有AI任务共享的控件：
 * - Python 环境选择（自动启动服务）
 * - 服务状态显示
 * - 模型选择（自动加载）+ 模型状态
 *
 * 这些控件在切换任务时保持状态，不会重新加载
 */
class EnvironmentServiceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironmentServiceWidget(QWidget *parent = nullptr);
    ~EnvironmentServiceWidget();

    /**
     * @brief 设置 YOLO 服务
     */
    void setYOLOService(Utils::YOLOService *service);

    /**
     * @brief 获取当前选择的模型路径
     */
    QString currentModelPath() const { return m_currentModelPath; }

    /**
     * @brief 更新模型列表（根据当前任务类型）
     */
    void updateModelList(Models::CVTask task);

    /**
     * @brief 更新服务状态显示
     */
    void updateServiceStatus();

    /**
     * @brief 更新执行按钮的启用状态
     */
    void updateRunButtonState(bool enabled);

signals:
    /**
     * @brief 日志消息信号
     */
    void logMessage(const QString &message);

    /**
     * @brief 服务启动成功信号
     */
    void serviceStarted();

    /**
     * @brief 服务停止信号
     */
    void serviceStopped();

    /**
     * @brief 模型加载成功信号
     */
    void modelLoaded(const QString &modelPath);

    /**
     * @brief 环境选择变化信号
     */
    void environmentChanged(const QString &envPath);

private slots:
    void onEnvironmentChanged(int index);
    void onBrowseModel();
    void onModelSelectionChanged(int index);

private:
    void setupUI();
    void connectSignals();
    void scanEnvironments();
    void tryAutoStartService();
    void tryAutoLoadModel();
    void updateModelStatus();

    // 环境选择控件
    QComboBox *m_envCombo;
    QLabel *m_lblServiceStatus;  // 服务状态

    // 模型选择控件
    QComboBox *m_cmbModelSelect;
    QPushButton *m_btnBrowseModel;
    QLabel *m_lblModelStatus;  // 模型状态

    // YOLO 服务引用（不拥有所有权）
    Utils::YOLOService *m_yoloService;

    // 当前状态
    QString m_currentModelPath;   // 当前选择的模型路径
    QString m_loadedModelPath;    // 已加载的模型路径
    Models::CVTask m_currentTask;

    // 保存的环境路径
    QString m_savedEnvPath;
};

} // namespace Widgets
} // namespace GenPreCVSystem

#endif // ENVIRONMENTSERVICEWIDGET_H
