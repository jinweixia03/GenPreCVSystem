#ifndef TASKTYPES_H
#define TASKTYPES_H

namespace GenPreCVSystem {
namespace Models {

/**
 * @brief CV任务类型枚举
 */
enum class CVTask {
    ImageClassification,      ///< 图像分类
    ObjectDetection,          ///< 目标检测
    SemanticSegmentation,     ///< 语义分割
    KeyPointDetection,        ///< 关键点检测
    ImageEnhancement,         ///< 图像增强
    ImageDenoising,           ///< 图像去噪
    EdgeDetection             ///< 边缘检测
};

/**
 * @brief 获取任务类型的中文名称
 * @param task 任务类型
 * @return 中文名称
 */
inline QString getTaskName(CVTask task)
{
    switch (task) {
        case CVTask::ImageClassification: return "图像分类";
        case CVTask::ObjectDetection: return "目标检测";
        case CVTask::SemanticSegmentation: return "语义分割";
        case CVTask::KeyPointDetection: return "关键点检测";
        case CVTask::ImageEnhancement: return "图像增强";
        case CVTask::ImageDenoising: return "图像去噪";
        case CVTask::EdgeDetection: return "边缘检测";
    }
    return QString();
}

} // namespace Models
} // namespace GenPreCVSystem

#endif // TASKTYPES_H
