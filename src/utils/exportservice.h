#ifndef EXPORTSERVICE_H
#define EXPORTSERVICE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QPair>

#include "yoloservice.h"

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 导出元数据结构
 */
struct ExportMetadata {
    QString imagePath;          // 图像路径
    QString modelName;          // 模型名称
    QString taskType;           // 任务类型
    double inferenceTime = 0.0; // 推理时间 (ms)
    QString timestamp;          // 时间戳
    int imageWidth = 0;         // 图像宽度
    int imageHeight = 0;        // 图像高度
};

/**
 * @brief 导出服务类
 *
 * 提供检测结果的导出功能，支持 JSON 和 CSV 格式
 */
class ExportService
{
public:
    /**
     * @brief 导出格式枚举
     */
    enum class Format {
        JSON,
        CSV
    };

    /**
     * @brief 导出检测结果
     * @param result 检测结果
     * @param metadata 元数据
     * @param filePath 导出文件路径
     * @param format 导出格式
     * @return 是否成功
     */
    static bool exportDetectionResult(const YOLODetectionResult &result,
                                       const ExportMetadata &metadata,
                                       const QString &filePath,
                                       Format format = Format::JSON);

    /**
     * @brief 导出分类结果
     */
    static bool exportClassificationResult(const YOLOClassificationResult &result,
                                            const ExportMetadata &metadata,
                                            const QString &filePath,
                                            Format format = Format::JSON);

    /**
     * @brief 导出关键点检测结果
     */
    static bool exportKeypointResult(const YOLOKeypointResult &result,
                                      const ExportMetadata &metadata,
                                      const QString &filePath,
                                      Format format = Format::JSON);

    /**
     * @brief 导出批量检测结果
     * @param results 批量结果（图像路径 -> 检测结果）
     * @param metadata 通用元数据
     * @param filePath 导出文件路径
     * @param format 导出格式
     * @return 是否成功
     */
    static bool exportBatchResults(const QVector<QPair<QString, YOLODetectionResult>> &results,
                                    const ExportMetadata &baseMetadata,
                                    const QString &filePath,
                                    Format format = Format::JSON);

    /**
     * @brief 获取导出文件过滤器
     */
    static QString getExportFilter();

    /**
     * @brief 根据扩展名获取格式
     */
    static Format formatFromExtension(const QString &extension);

private:
    // JSON 导出辅助方法
    static QJsonObject metadataToJson(const ExportMetadata &metadata);
    static QJsonObject detectionToJson(const YOLODetection &det);
    static QJsonObject classificationToJson(const ClassificationResult &cls);
    static QJsonObject keypointToJson(const KeypointData &kp);

    // CSV 导出辅助方法
    static QString detectionToCsvHeader();
    static QString detectionToCsvRow(const YOLODetection &det, const ExportMetadata &metadata);
    static QString classificationToCsvHeader();
    static QString classificationToCsvRow(const ClassificationResult &cls, const ExportMetadata &metadata);
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // EXPORTSERVICE_H
