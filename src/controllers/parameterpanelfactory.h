#ifndef PARAMETERPANELFACTORY_H
#define PARAMETERPANELFACTORY_H

#include <QWidget>
#include <QString>

#include "../models/tasktypes.h"

namespace GenPreCVSystem {
namespace Controllers {

/**
 * @brief 参数面板工厂类
 *
 * 根据不同的任务类型创建对应的参数面板UI
 */
class ParameterPanelFactory
{
public:
    /**
     * @brief 创建参数面板
     * @param task 任务类型
     * @return 参数面板widget
     */
    static QWidget* createParameterPanel(Models::CVTask task);

private:
    // 各任务类型的参数面板创建函数
    static QWidget* createImageClassificationParams();
    static QWidget* createObjectDetectionParams();
    static QWidget* createSemanticSegmentationParams();
    static QWidget* createKeyPointDetectionParams();
    static QWidget* createImageEnhancementParams();
    static QWidget* createImageDenoisingParams();
    static QWidget* createEdgeDetectionParams();
};

} // namespace Controllers
} // namespace GenPreCVSystem

#endif // PARAMETERPANELFACTORY_H
