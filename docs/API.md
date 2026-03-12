# API 文档

本文档详细介绍 GenPreCVSystem 的 C++ API 和 Python 服务接口。

## 目录

- [C++ API](#c-api)
  - [DLService](#dlservice)
  - [ImageProcessor](#imageprocessor)
  - [FileUtils](#fileutils)
- [Python 服务接口](#python-服务接口)
  - [通信协议](#通信协议)
  - [命令列表](#命令列表)
- [数据类型](#数据类型)

---

## C++ API

### DLService

深度学习推理服务，封装 Python 后端通信。

#### 头文件

```cpp
#include "services/inference/dlservice.h"
```

#### 命名空间

```cpp
GenPreCVSystem::Utils
```

#### 构造函数

```cpp
DLService(QObject* parent = nullptr);
```

#### 公共方法

##### 环境管理

```cpp
// 扫描可用的 Python 环境
QVector<PythonEnvironment> scanEnvironments();

// 扫描特定路径下的环境
QVector<PythonEnvironment> scanEnvironments(const QStringList& searchPaths);

// 设置当前使用的 Python 路径
void setEnvironmentPath(const QString& path);

// 获取当前环境路径
QString environmentPath() const;
```

##### 服务生命周期

```cpp
// 启动 Python 服务
bool start();

// 停止服务
void stop();

// 检查服务是否运行
bool isRunning() const;

// 获取服务信息
QJsonObject getServiceInfo();
```

##### 模型管理

```cpp
// 加载模型
bool loadModel(const QString& modelPath, const QString& labelsPath = QString());

// 卸载模型
void unloadModel();

// 检查模型是否已加载
bool isModelLoaded() const;

// 获取模型路径
QString modelPath() const;

// 获取类别名称列表
QStringList classNames() const;
```

##### 推理方法

```cpp
// 目标检测
DLDetectionResult detect(const QString& imagePath,
                         float confThreshold = 0.25f,
                         float iouThreshold = 0.45f,
                         int imageSize = 640);

// 语义分割
DLSegmentationResult segment(const QString& imagePath,
                              float confThreshold = 0.25f,
                              float iouThreshold = 0.45f,
                              int imageSize = 640);

// 图像分类
DLClassificationResult classify(const QString& imagePath,
                                 int topK = 5,
                                 int imageSize = 640);

// 关键点检测
DLKeypointResult keypoint(const QString& imagePath,
                           float confThreshold = 0.25f,
                           float iouThreshold = 0.45f,
                           int imageSize = 640);
```

#### 信号

```cpp
// 服务启动完成
void serviceStarted();

// 服务已停止
void serviceStopped();

// 模型加载完成
void modelLoaded(const QString& modelPath);

// 模型已卸载
void modelUnloaded();

// 检测到环境错误
void environmentError(const QString& error);

// 检测到 Python 错误
void pythonError(const QString& error);

// 推理完成
void inferenceCompleted(const QJsonObject& result);
```

#### 使用示例

```cpp
#include "services/inference/dlservice.h"
#include <QDebug>

using namespace GenPreCVSystem::Utils;

// 创建服务实例
DLService* service = new DLService(this);

// 连接信号
connect(service, &DLService::serviceStarted, []() {
    qDebug() << "服务已启动";
});

connect(service, &DLService::modelLoaded, [](const QString& path) {
    qDebug() << "模型已加载:" << path;
});

// 扫描环境
QVector<PythonEnvironment> envs = service->scanEnvironments();
for (const auto& env : envs) {
    qDebug() << "环境:" << env.name << "路径:" << env.pythonPath;
}

// 启动服务
if (!envs.isEmpty()) {
    service->setEnvironmentPath(envs.first().pythonPath);
    if (service->start()) {
        // 加载模型
        if (service->loadModel("yolov8n.pt")) {
            // 执行检测
            auto result = service->detect("image.jpg", 0.25f, 0.45f, 640);
            if (result.success) {
                for (const auto& det : result.detections) {
                    qDebug() << "检测到:" << det.label
                             << "置信度:" << det.confidence
                             << "位置:" << det.bbox;
                }
            }
        }
    }
}
```

---

### ImageProcessor

图像处理工具类。

#### 头文件

```cpp
#include "services/image/imageprocessor.h"
```

#### 静态方法

```cpp
// 基础变换
static QImage toGrayscale(const QImage& image);
static QImage invert(const QImage& image);
static QImage binarize(const QImage& image, int threshold = 128);
static QImage rotateLeft(const QImage& image);
static QImage rotateRight(const QImage& image);
static QImage flipHorizontal(const QImage& image);
static QImage flipVertical(const QImage& image);

// 滤镜效果
static QImage blur(const QImage& image, int radius = 5);
static QImage sharpen(const QImage& image);
static QImage edgeDetection(const QImage& image, EdgeAlgorithm algo);

// 参数调整
static QImage adjustBrightness(const QImage& image, int delta);
static QImage adjustContrast(const QImage& image, float factor);
static QImage adjustSaturation(const QImage& image, float factor);
static QImage adjustSharpness(const QImage& image, float factor);
```

#### 使用示例

```cpp
#include "services/image/imageprocessor.h"

using namespace GenPreCVSystem::Utils;

// 加载图片
QImage image("photo.jpg");

// 灰度化
QImage gray = ImageProcessor::toGrayscale(image);

// 旋转
QImage rotated = ImageProcessor::rotateRight(image);

// 调整亮度
QImage brighter = ImageProcessor::adjustBrightness(image, 30);

// 边缘检测
QImage edges = ImageProcessor::edgeDetection(image, EdgeAlgorithm::Canny);
```

---

### FileUtils

文件操作工具类。

#### 头文件

```cpp
#include "services/io/fileutils.h"
```

#### 静态方法

```cpp
// 路径操作
static QString getFileExtension(const QString& path);
static QString getFileName(const QString& path);
static QString getDirectory(const QString& path);
static QString normalizePath(const QString& path);

// 文件检查
static bool exists(const QString& path);
static bool isImageFile(const QString& path);
static qint64 getFileSize(const QString& path);

// 目录操作
static bool ensureDirectory(const QString& path);
static QStringList listImageFiles(const QString& directory);
static QStringList listFiles(const QString& directory, const QStringList& filters);

// 导出操作
static bool exportToJson(const QString& path, const QJsonObject& data);
static bool exportToCsv(const QString& path, const QList<QStringList>& data);
static bool exportToXml(const QString& path, const QDomDocument& doc);
```

---

## Python 服务接口

### 通信协议

C++ 前端与 Python 后端通过 **stdin/stdout** 使用 **JSON** 格式通信。

#### 消息格式

```
[消息长度(4字节)]\n[JSON消息]\n
示例:
42\n{"command":"get_info","params":{}}\n
注：当前实现使用简单的换行分隔，非二进制长度头
```

### 命令列表

#### 服务管理

**获取服务信息**
```json
// 请求
{
    "command": "get_info"
}

// 响应
{
    "success": true,
    "message": "服务信息",
    "data": {
        "has_ultralytics": true,
        "has_torch": true,
        "device": "cuda",
        "default_conf": 0.25,
        "default_iou": 0.45,
        "default_image_size": 640
    }
}
```

**健康检查**
```json
// 请求
{
    "command": "health"
}

// 响应
{
    "success": true,
    "message": "服务运行正常"
}
```

**退出服务**
```json
// 请求
{
    "command": "exit"
}

// 响应
{
    "success": true,
    "message": "服务已退出"
}
```

#### 模型管理

**加载模型**
```json
// 请求
{
    "command": "load_model",
    "model_path": "path/to/model.pt",
    "labels_path": "path/to/labels.txt"
}

// 响应
{
    "success": true,
    "message": "模型加载成功",
    "data": {
        "num_classes": 80,
        "class_names": ["person", "car", ...]
    }
}
```

**获取模型信息**
```json
// 请求
{
    "command": "get_model_info"
}

// 响应
{
    "success": true,
    "message": "模型信息",
    "data": {
        "loaded": true,
        "path": "path/to/model.pt",
        "task": "detect",
        "num_classes": 80
    }
}
```

#### 推理命令

**目标检测**
```json
// 请求
{
    "command": "detect",
    "image_path": "path/to/image.jpg",
    "conf_threshold": 0.25,
    "iou_threshold": 0.45,
    "image_size": 640
}

// 响应
{
    "success": true,
    "message": "检测完成",
    "data": {
        "inference_time": 0.045,
        "num_detections": 3,
        "detections": [
            {
                "label": "person",
                "confidence": 0.92,
                "bbox": [100, 200, 150, 300]
            }
        ]
    }
}
```

**语义分割**
```json
// 请求
{
    "command": "segment",
    "image_path": "path/to/image.jpg",
    "conf_threshold": 0.25,
    "iou_threshold": 0.45,
    "image_size": 640
}

// 响应
{
    "success": true,
    "message": "分割完成",
    "data": {
        "inference_time": 0.052,
        "num_masks": 2,
        "masks": [
            {
                "label": "car",
                "confidence": 0.89,
                "bbox": [100, 200, 300, 400],
                "polygon": [[100, 200], [300, 200], [300, 400], [100, 400]]
            }
        ]
    }
}
```

**图像分类**
```json
// 请求
{
    "command": "classify",
    "image_path": "path/to/image.jpg",
    "top_k": 5,
    "image_size": 640
}

// 响应
{
    "success": true,
    "message": "分类完成",
    "data": {
        "inference_time": 0.023,
        "top_predictions": [
            {"label": "cat", "confidence": 0.95},
            {"label": "dog", "confidence": 0.03}
        ]
    }
}
```

**关键点检测**
```json
// 请求
{
    "command": "keypoint",
    "image_path": "path/to/image.jpg",
    "conf_threshold": 0.25,
    "iou_threshold": 0.45,
    "image_size": 640
}

// 响应
{
    "success": true,
    "message": "关键点检测完成",
    "data": {
        "inference_time": 0.048,
        "num_poses": 1,
        "poses": [
            {
                "bbox": [100, 200, 200, 400],
                "confidence": 0.91,
                "keypoints": [
                    {"x": 150, "y": 220, "confidence": 0.95, "name": "nose"},
                    {"x": 140, "y": 230, "confidence": 0.92, "name": "left_eye"}
                ]
            }
        ]
    }
}
```

---

## 数据类型

### PythonEnvironment

```cpp
struct PythonEnvironment {
    QString name;           // 环境名称
    QString pythonPath;     // Python 可执行文件路径
    QString type;           // 类型: "conda", "venv", "system"
    bool isValid;           // 是否有效
    bool hasUltralytics;    // 是否安装 ultralytics
    bool hasTorch;          // 是否安装 torch
    bool hasCuda;           // 是否支持 CUDA
    QString version;        // Python 版本
};
```

### DetectionResult

```cpp
struct Detection {
    QString label;          // 类别标签
    float confidence;       // 置信度 (0-1)
    QRectF bbox;            // 边界框 [x, y, width, height]
    int classId;            // 类别 ID
};

struct DetectionResult {
    bool success;                       // 是否成功
    QString message;                    // 消息
    float inferenceTime;                // 推理时间 (秒)
    QVector<Detection> detections;      // 检测结果列表
};
```

### SegmentationResult

```cpp
struct Segmentation {
    QString label;          // 类别标签
    float confidence;       // 置信度
    QRectF bbox;            // 边界框
    QPolygonF polygon;      // 分割多边形
    QImage mask;            // 掩膜图像
    int classId;            // 类别 ID
};

struct SegmentationResult {
    bool success;
    QString message;
    float inferenceTime;
    QVector<Segmentation> masks;
};
```

### ClassificationResult

```cpp
struct ClassPrediction {
    QString label;          // 类别标签
    float confidence;       // 置信度
    int classId;            // 类别 ID
};

struct ClassificationResult {
    bool success;
    QString message;
    float inferenceTime;
    QVector<ClassPrediction> predictions;
};
```

### KeypointResult

```cpp
struct Keypoint {
    float x;                // X 坐标
    float y;                // Y 坐标
    float confidence;       // 置信度
    QString name;           // 关键点名称
};

struct Pose {
    QRectF bbox;            // 人体边界框
    float confidence;       // 整体置信度
    QVector<Keypoint> keypoints;  // 关键点列表
};

struct KeypointResult {
    bool success;
    QString message;
    float inferenceTime;
    QVector<Pose> poses;
};
```

---

## 错误处理

### 错误响应格式

```json
{
    "success": false,
    "message": "错误描述",
    "error_code": "ERROR_CODE",
    "details": "详细错误信息"
}
```

### 错误码列表

| 错误码 | 描述 |
|--------|------|
| `INVALID_COMMAND` | 未知命令 |
| `MODEL_NOT_LOADED` | 模型未加载 |
| `IMAGE_LOAD_FAILED` | 图片加载失败 |
| `INFERENCE_FAILED` | 推理失败 |
| `INVALID_PARAMETERS` | 参数无效 |
| `SERVICE_NOT_STARTED` | 服务未启动 |
| `ENVIRONMENT_ERROR` | 环境错误 |

---

## 更多示例

查看 `tests/` 目录获取更多使用示例。
