#include "exportservice.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

namespace GenPreCVSystem {
namespace Utils {

QString ExportService::getExportFilter()
{
    return "JSON 文件 (*.json);;CSV 文件 (*.csv)";
}

ExportService::Format ExportService::formatFromExtension(const QString &extension)
{
    QString ext = extension.toLower();
    if (ext == "csv") {
        return Format::CSV;
    }
    return Format::JSON;  // 默认 JSON
}

bool ExportService::exportDetectionResult(const YOLODetectionResult &result,
                                           const ExportMetadata &metadata,
                                           const QString &filePath,
                                           Format format)
{
    if (format == Format::JSON) {
        QJsonObject root;

        // 元数据
        root["metadata"] = metadataToJson(metadata);

        // 检测结果
        QJsonArray detectionsArray;
        for (const YOLODetection &det : result.detections) {
            detectionsArray.append(detectionToJson(det));
        }
        root["detections"] = detectionsArray;

        // 统计信息
        QJsonObject statistics;
        statistics["total"] = result.detections.size();
        statistics["inferenceTime"] = result.inferenceTime;
        statistics["success"] = result.success;
        root["statistics"] = statistics;

        QJsonDocument doc(root);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    } else {
        // CSV 格式
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);

        // 写入表头
        stream << detectionToCsvHeader() << "\n";

        // 写入数据行
        for (const YOLODetection &det : result.detections) {
            stream << detectionToCsvRow(det, metadata) << "\n";
        }

        file.close();
        return true;
    }
}

bool ExportService::exportClassificationResult(const YOLOClassificationResult &result,
                                                const ExportMetadata &metadata,
                                                const QString &filePath,
                                                Format format)
{
    if (format == Format::JSON) {
        QJsonObject root;

        root["metadata"] = metadataToJson(metadata);

        QJsonArray classificationsArray;
        for (const ClassificationResult &cls : result.classifications) {
            classificationsArray.append(classificationToJson(cls));
        }
        root["classifications"] = classificationsArray;

        if (result.classifications.size() > 0) {
            root["topPrediction"] = classificationToJson(result.topPrediction);
        }

        QJsonObject statistics;
        statistics["count"] = result.classifications.size();
        statistics["inferenceTime"] = result.inferenceTime;
        root["statistics"] = statistics;

        QJsonDocument doc(root);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    } else {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);

        stream << classificationToCsvHeader() << "\n";
        for (const ClassificationResult &cls : result.classifications) {
            stream << classificationToCsvRow(cls, metadata) << "\n";
        }

        file.close();
        return true;
    }
}

bool ExportService::exportKeypointResult(const YOLOKeypointResult &result,
                                          const ExportMetadata &metadata,
                                          const QString &filePath,
                                          Format format)
{
    if (format == Format::JSON) {
        QJsonObject root;

        root["metadata"] = metadataToJson(metadata);

        QJsonArray detectionsArray;
        for (const YOLOKeypointDetection &det : result.detections) {
            QJsonObject detObj;
            detObj["classId"] = det.classId;
            detObj["label"] = det.label;
            detObj["confidence"] = det.confidence;

            QJsonObject bbox;
            bbox["x"] = det.x;
            bbox["y"] = det.y;
            bbox["width"] = det.width;
            bbox["height"] = det.height;
            detObj["bbox"] = bbox;

            QJsonArray keypointsArray;
            for (const KeypointData &kp : det.keypoints) {
                keypointsArray.append(keypointToJson(kp));
            }
            detObj["keypoints"] = keypointsArray;

            detectionsArray.append(detObj);
        }
        root["detections"] = detectionsArray;

        QJsonObject statistics;
        statistics["total"] = result.detections.size();
        statistics["inferenceTime"] = result.inferenceTime;
        root["statistics"] = statistics;

        QJsonDocument doc(root);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    } else {
        // CSV 格式对关键点不太友好，使用简化的表格
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);

        stream << "image_path,object_id,label,confidence,keypoint_id,keypoint_x,keypoint_y,keypoint_confidence\n";

        int objId = 0;
        for (const YOLOKeypointDetection &det : result.detections) {
            for (const KeypointData &kp : det.keypoints) {
                stream << QString("%1,%2,%3,%4,%5,%6,%7,%8\n")
                    .arg(metadata.imagePath)
                    .arg(objId)
                    .arg(det.label)
                    .arg(det.confidence)
                    .arg(kp.id)
                    .arg(kp.x)
                    .arg(kp.y)
                    .arg(kp.confidence);
            }
            objId++;
        }

        file.close();
        return true;
    }
}

bool ExportService::exportBatchResults(const QVector<QPair<QString, YOLODetectionResult>> &results,
                                        const ExportMetadata &baseMetadata,
                                        const QString &filePath,
                                        Format format)
{
    if (format == Format::JSON) {
        QJsonObject root;

        // 批处理元数据
        QJsonObject meta = metadataToJson(baseMetadata);
        meta["batchSize"] = results.size();
        meta["exportTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        root["metadata"] = meta;

        // 汇总统计
        int totalDetections = 0;
        double totalTime = 0.0;
        int successCount = 0;

        QJsonArray resultsArray;
        for (const auto &pair : results) {
            const QString &imagePath = pair.first;
            const YOLODetectionResult &result = pair.second;

            QJsonObject itemObj;
            itemObj["imagePath"] = imagePath;
            itemObj["success"] = result.success;
            itemObj["message"] = result.message;
            itemObj["detectionCount"] = result.detections.size();
            itemObj["inferenceTime"] = result.inferenceTime;

            QJsonArray detectionsArray;
            for (const YOLODetection &det : result.detections) {
                detectionsArray.append(detectionToJson(det));
            }
            itemObj["detections"] = detectionsArray;

            resultsArray.append(itemObj);

            totalDetections += result.detections.size();
            totalTime += result.inferenceTime;
            if (result.success) successCount++;
        }
        root["results"] = resultsArray;

        QJsonObject summary;
        summary["totalImages"] = results.size();
        summary["successCount"] = successCount;
        summary["failCount"] = results.size() - successCount;
        summary["totalDetections"] = totalDetections;
        summary["totalTime"] = totalTime;
        summary["averageTime"] = results.isEmpty() ? 0 : totalTime / results.size();
        root["summary"] = summary;

        QJsonDocument doc(root);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    } else {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);

        stream << detectionToCsvHeader() << "\n";

        for (const auto &pair : results) {
            ExportMetadata meta = baseMetadata;
            meta.imagePath = pair.first;
            for (const YOLODetection &det : pair.second.detections) {
                stream << detectionToCsvRow(det, meta) << "\n";
            }
        }

        file.close();
        return true;
    }
}

// ========== 私有辅助方法 ==========

QJsonObject ExportService::metadataToJson(const ExportMetadata &metadata)
{
    QJsonObject obj;
    obj["imagePath"] = metadata.imagePath;
    obj["modelName"] = metadata.modelName;
    obj["taskType"] = metadata.taskType;
    obj["inferenceTime"] = metadata.inferenceTime;
    obj["timestamp"] = metadata.timestamp;

    QJsonObject imageSize;
    imageSize["width"] = metadata.imageWidth;
    imageSize["height"] = metadata.imageHeight;
    obj["imageSize"] = imageSize;

    return obj;
}

QJsonObject ExportService::detectionToJson(const YOLODetection &det)
{
    QJsonObject obj;
    obj["classId"] = det.classId;
    obj["label"] = det.label;
    obj["confidence"] = det.confidence;

    QJsonObject bbox;
    bbox["x"] = det.x;
    bbox["y"] = det.y;
    bbox["width"] = det.width;
    bbox["height"] = det.height;
    obj["bbox"] = bbox;

    // 如果有掩码多边形
    if (!det.maskPolygon.isEmpty()) {
        QJsonArray maskArray;
        for (const MaskPoint &pt : det.maskPolygon) {
            QJsonObject ptObj;
            ptObj["x"] = pt.x;
            ptObj["y"] = pt.y;
            maskArray.append(ptObj);
        }
        obj["maskPolygon"] = maskArray;
    }

    return obj;
}

QJsonObject ExportService::classificationToJson(const ClassificationResult &cls)
{
    QJsonObject obj;
    obj["rank"] = cls.rank;
    obj["classId"] = cls.classId;
    obj["label"] = cls.label;
    obj["confidence"] = cls.confidence;
    return obj;
}

QJsonObject ExportService::keypointToJson(const KeypointData &kp)
{
    QJsonObject obj;
    obj["id"] = kp.id;
    obj["x"] = kp.x;
    obj["y"] = kp.y;
    obj["confidence"] = kp.confidence;
    return obj;
}

QString ExportService::detectionToCsvHeader()
{
    return "image_path,class_id,label,confidence,x,y,width,height,timestamp";
}

QString ExportService::detectionToCsvRow(const YOLODetection &det, const ExportMetadata &metadata)
{
    return QString("%1,%2,%3,%4,%5,%6,%7,%8,%9")
        .arg(metadata.imagePath)
        .arg(det.classId)
        .arg(det.label)
        .arg(det.confidence)
        .arg(det.x)
        .arg(det.y)
        .arg(det.width)
        .arg(det.height)
        .arg(metadata.timestamp);
}

QString ExportService::classificationToCsvHeader()
{
    return "image_path,rank,class_id,label,confidence,timestamp";
}

QString ExportService::classificationToCsvRow(const ClassificationResult &cls, const ExportMetadata &metadata)
{
    return QString("%1,%2,%3,%4,%5,%6")
        .arg(metadata.imagePath)
        .arg(cls.rank)
        .arg(cls.classId)
        .arg(cls.label)
        .arg(cls.confidence)
        .arg(metadata.timestamp);
}

} // namespace Utils
} // namespace GenPreCVSystem
