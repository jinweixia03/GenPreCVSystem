#include "taskcontroller.h"
#include "../models/tasktypes.h"
#include "parameterpanelfactory.h"
#include <QScrollArea>

namespace GenPreCVSystem {
namespace Controllers {

TaskController::TaskController(QObject *parent)
    : QObject(parent)
    , m_paramScrollArea(nullptr)
    , m_taskActionGroup(nullptr)
    , m_currentTask(Models::CVTask::ImageClassification)
{
}

TaskController::~TaskController()
{
}

void TaskController::setParameterScrollArea(QScrollArea *scrollArea)
{
    m_paramScrollArea = scrollArea;
}

void TaskController::setTaskActionGroup(QActionGroup *actionGroup)
{
    m_taskActionGroup = actionGroup;
}

void TaskController::switchTask(Models::CVTask task)
{
    m_currentTask = task;
    updateParameterPanel(task);

    emit taskChanged(task);
    emit logMessage(QString("已切换任务: %1").arg(Models::getTaskName(task)));
}

void TaskController::updateParameterPanel(Models::CVTask task)
{
    clearParameterPanel();

    QWidget *content = ParameterPanelFactory::createParameterPanel(task);

    if (content && m_paramScrollArea) {
        m_paramScrollArea->setWidget(content);
        m_paramScrollArea->setWidgetResizable(true);
    }
}

void TaskController::clearParameterPanel()
{
    if (m_paramScrollArea && m_paramScrollArea->widget()) {
        delete m_paramScrollArea->widget();
        m_paramScrollArea->setWidget(nullptr);
    }
}

} // namespace Controllers
} // namespace GenPreCVSystem
