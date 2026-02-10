#ifndef TASKCONTROLLER_H
#define TASKCONTROLLER_H

#include <QObject>
#include <QActionGroup>
#include <QScrollArea>

#include "../models/tasktypes.h"

namespace GenPreCVSystem {
namespace Controllers {

/**
 * @brief 任务控制器
 *
 * 管理CV任务类型的切换和参数面板更新
 */
class TaskController : public QObject
{
    Q_OBJECT

public:
    explicit TaskController(QObject *parent = nullptr);
    ~TaskController();

    /**
     * @brief 设置参数面板滚动区域
     */
    void setParameterScrollArea(QScrollArea *scrollArea);

    /**
     * @brief 设置任务动作组
     */
    void setTaskActionGroup(QActionGroup *actionGroup);

    /**
     * @brief 切换任务类型
     */
    void switchTask(Models::CVTask task);

    /**
     * @brief 获取当前任务类型
     */
    Models::CVTask currentTask() const { return m_currentTask; }

signals:
    /**
     * @brief 任务已切换信号
     */
    void taskChanged(Models::CVTask task);

    /**
     * @brief 日志消息信号
     */
    void logMessage(const QString &message);

private:
    void updateParameterPanel(Models::CVTask task);
    void clearParameterPanel();

    QScrollArea *m_paramScrollArea;
    QActionGroup *m_taskActionGroup;
    Models::CVTask m_currentTask;
};

} // namespace Controllers
} // namespace GenPreCVSystem

#endif // TASKCONTROLLER_H
