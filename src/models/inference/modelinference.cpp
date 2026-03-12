#include "modelinference.h"
#include <QElapsedTimer>
#include <QDebug>

#ifdef LIBTORCH_FOUND
#include <torch/torch.h>
#endif

namespace GenPreCVSystem {
namespace Models {

#ifdef LIBTORCH_FOUND

// ============================================================================
// TorchModelInference 实现
// ============================================================================

TorchModelInference::TorchModelInference()
    : m_device(torch::kCPU)
    , m_modelLoaded(false)
    , m_mean({0.485f, 0.456f, 0.406f})  // ImageNet 均值
    , m_std({0.229f, 0.224f, 0.225f})    // ImageNet 标准差
{
}

TorchModelInference::~TorchModelInference()
{
}

bool TorchModelInference::loadModel(const QString &modelPath)
{
    try {
        qDebug() << "Loading model from:" << modelPath;

        // 加载 TorchScript 模型
        m_module = std::make_unique<torch::jit::script::Module>(
            torch::jit::load(modelPath.toStdString())
        );

        // 将模型移动到指定设备
        m_module->to(m_device);
        m_module->eval();

        m_modelLoaded = true;
        qDebug() << "Model loaded successfully";
        return true;

    } catch (const c10::Error &e) {
        qWarning() << "Error loading model:" << e.what();
        m_modelLoaded = false;
        return false;
    } catch (const std::exception &e) {
        qWarning() << "Error loading model:" << e.what();
        m_modelLoaded = false;
        return false;
    }
}

bool TorchModelInference::isModelLoaded() const
{
    return m_modelLoaded && m_module != nullptr;
}

bool TorchModelInference::setDevice(bool useGPU)
{
    try {
        if (useGPU && torch::cuda::is_available()) {
            m_device = torch::kCUDA;
            qDebug() << "Using CUDA device";
        } else {
            m_device = torch::kCPU;
            if (useGPU) {
                qWarning() << "CUDA is not available, using CPU";
            }
        }

        // 如果模型已加载，移动到新设备
        if (m_module) {
            m_module->to(m_device);
        }

        return true;
    } catch (const std::exception &e) {
        qWarning() << "Error setting device:" << e.what();
        return false;
    }
}

QMap<QString, QVariant> TorchModelInference::getModelInfo() const
{
    QMap<QString, QVariant> info;
    info["modelLoaded"] = m_modelLoaded;
    info["device"] = (m_device.is_cuda()) ? "CUDA" : "CPU";
    info["inputSize"] = QSize(m_inputWidth, m_inputHeight);
    return info;
}

void TorchModelInference::setParameters(const QMap<QString, QVariant> &params)
{
    if (params.contains("inputWidth")) {
        m_inputWidth = params["inputWidth"].toInt();
    }
    if (params.contains("inputHeight")) {
        m_inputHeight = params["inputHeight"].toInt();
    }
    if (params.contains("mean")) {
        m_mean = params["mean"].value<QVector<float>>();
    }
    if (params.contains("std")) {
        m_std = params["std"].value<QVector<float>>();
    }
}

QMap<QString, QVariant> TorchModelInference::getParameters() const
{
    QMap<QString, QVariant> params;
    params["inputWidth"] = m_inputWidth;
    params["inputHeight"] = m_inputHeight;
    params["mean"] = QVariant::fromValue(m_mean);
    params["std"] = QVariant::fromValue(m_std);
    return params;
}

// ============================================================================
// ImageClassificationModel 实现
// ============================================================================

ImageClassificationModel::ImageClassificationModel()
    : TorchModelInference()
{
}

ClassificationResult ImageClassificationModel::classify(const QImage &image)
{
    ClassificationResult result;

    if (!isModelLoaded()) {
        result.success = false;
        result.errorMessage = "Model not loaded";
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    try {
        // 预处理图像
        torch::Tensor inputTensor = preprocessImage(image);

        // 执行推理
        at::Tensor output = m_module->forward({inputTensor}).toTensor();

        // 后处理
        result = static_cast<ClassificationResult>(
            postprocessOutput(output)
        );

        result.inferenceTime = timer.elapsed();
        result.success = true;

    } catch (const std::exception &e) {
        result.success = false;
        result.errorMessage = QString("Inference error: %1").arg(e.what());
        qWarning() << "Inference error:" << e.what();
    }

    return result;
}

InferenceResult ImageClassificationModel::infer(const QImage &image)
{
    return classify(image);
}

void ImageClassificationModel::setClassLabels(const QStringList &labels)
{
    m_classLabels = labels;
}

void ImageClassificationModel::setTopK(int k)
{
    m_topK = qMax(1, k);
}

torch::Tensor ImageClassificationModel::preprocessImage(const QImage &image)
{
    // 转换为 RGB
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);

    // 调整大小
    QImage resizedImage = rgbImage.scaled(
        m_inputWidth, m_inputHeight,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    // 转换为张量
    torch::Tensor tensor = torch::from_blob(
        resizedImage.bits(),
        {1, m_inputHeight, m_inputWidth, 3},
        torch::kUInt8
    );

    // 转换布局和类型: HWC -> CHW, uint8 -> float
    tensor = tensor.permute({0, 3, 1, 2}).to(torch::kFloat32) / 255.0;

    // 归一化
    torch::Tensor meanTensor = torch::from_blob(
        m_mean.data(), {3, 1, 1}, torch::kFloat32
    ).clone();

    torch::Tensor stdTensor = torch::from_blob(
        m_std.data(), {3, 1, 1}, torch::kFloat32
    ).clone();

    tensor = (tensor - meanTensor) / stdTensor;

    // 移动到设备
    tensor = tensor.to(m_device);

    return tensor;
}

InferenceResult ImageClassificationModel::postprocessOutput(const torch::Tensor &output)
{
    ClassificationResult result;

    try {
        // 移动到 CPU 并转换为 softmax
        torch::Tensor probabilities = torch::softmax(output, 1).squeeze(0);

        // 获取 Top-K
        torch::Tensor topKResult = std::get<0>(torch::topk(probabilities, m_topK));
        torch::Tensor topKIndices = std::get<1>(torch::topk(probabilities, m_topK));

        // 提取结果
        auto probsAccessor = topKResult.accessor<float, 1>();
        auto indicesAccessor = topKIndices.accessor<int64_t, 1>();

        for (int i = 0; i < m_topK; ++i) {
            int classId = static_cast<int>(indicesAccessor[i]);
            float prob = probsAccessor[i];

            QString className = m_classLabels.isEmpty()
                ? QString("Class_%1").arg(classId)
                : m_classLabels.value(classId, QString("Class_%1").arg(classId));

            result.topK.append({className, prob});

            // 第一个结果作为主要预测
            if (i == 0) {
                result.classId = classId;
                result.className = className;
                result.confidence = prob;
            }
        }

        result.success = true;

    } catch (const std::exception &e) {
        result.success = false;
        result.errorMessage = QString("Postprocessing error: %1").arg(e.what());
    }

    return result;
}

#endif // LIBTORCH_FOUND

} // namespace Models
} // namespace GenPreCVSystem
