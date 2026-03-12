#ifndef MODELINFERENCE_H
#define MODELINFERENCE_H

#include <QImage>
#include <QString>
#include <QMap>
#include <QVariant>
#include <memory>

// 检查是否启用 LibTorch
#ifdef LIBTORCH_FOUND
#include <torch/torch.h>
#include <torch/script.h>
#endif

namespace GenPreCVSystem {
namespace Models {

/**
 * @brief 模型推理结果基类
 */
struct InferenceResult {
    bool success = false;           ///< 推理是否成功
    QString errorMessage;           ///< 错误信息
    QMap<QString, QVariant> data;   ///< 结果数据
    double inferenceTime = 0.0;     ///< 推理耗时（毫秒）
};

/**
 * @brief 图像分类结果
 */
struct ClassificationResult : public InferenceResult {
    QString className;              ///< 类别名称
    int classId = -1;               ///< 类别ID
    float confidence = 0.0f;        ///< 置信度
    QVector<QPair<QString, float>> topK;  ///< Top-K 预测结果
};

/**
 * @brief 目标检测结果
 */
struct DetectionResult : public InferenceResult {
    struct BoundingBox {
        int x, y, width, height;    ///< 边界框坐标
        QString label;              ///< 类别标签
        float confidence;           ///< 置信度
    };

    QVector<BoundingBox> boxes;     ///< 检测到的边界框
};

/**
 * @brief 语义分割结果
 */
struct SegmentationResult : public InferenceResult {
    QImage maskImage;               ///< 分割掩码图像
    QMap<int, QString> labelMap;    ///< 标签映射
};

/**
 * @brief 模型推理接口基类
 *
 * 提供统一的模型推理接口，支持加载 PyTorch 模型并进行推理
 */
class ModelInference
{
public:
    virtual ~ModelInference() = default;

    /**
     * @brief 加载模型
     * @param modelPath 模型文件路径（.pt 或 .pth 文件）
     * @return 是否加载成功
     */
    virtual bool loadModel(const QString &modelPath) = 0;

    /**
     * @brief 执行推理
     * @param image 输入图像
     * @return 推理结果
     */
    virtual InferenceResult infer(const QImage &image) = 0;

    /**
     * @brief 检查模型是否已加载
     * @return 是否已加载
     */
    virtual bool isModelLoaded() const = 0;

    /**
     * @brief 获取模型信息
     * @return 模型信息字典
     */
    virtual QMap<QString, QVariant> getModelInfo() const = 0;

    /**
     * @brief 设置推理参数
     * @param params 参数字典
     */
    virtual void setParameters(const QMap<QString, QVariant> &params) = 0;

    /**
     * @brief 获取推理参数
     * @return 参数字典
     */
    virtual QMap<QString, QVariant> getParameters() const = 0;

    /**
     * @brief 设置是否使用 GPU
     * @param useGPU 是否使用 GPU
     * @return 是否设置成功
     */
    virtual bool setDevice(bool useGPU) = 0;
};

#ifdef LIBTORCH_FOUND

/**
 * @brief LibTorch 模型推理基类
 *
 * 基于 LibTorch 的模型推理实现
 */
class TorchModelInference : public ModelInference
{
public:
    TorchModelInference();
    virtual ~TorchModelInference();

    bool loadModel(const QString &modelPath) override;
    bool isModelLoaded() const override;
    bool setDevice(bool useGPU) override;

    QMap<QString, QVariant> getModelInfo() const override;
    void setParameters(const QMap<QString, QVariant> &params) override;
    QMap<QString, QVariant> getParameters() const override;

protected:
    /**
     * @brief 预处理图像
     * @param image 输入图像
     * @return 处理后的张量
     */
    virtual torch::Tensor preprocessImage(const QImage &image) = 0;

    /**
     * @brief 后处理模型输出
     * @param output 模型输出
     * @return 推理结果
     */
    virtual InferenceResult postprocessOutput(const torch::Tensor &output) = 0;

    std::unique_ptr<torch::jit::script::Module> m_module;  ///< JIT 编译的模型
    torch::Device m_device;                                 ///< 计算设备
    bool m_modelLoaded = false;                             ///< 模型是否已加载

    // 预处理参数
    int m_inputWidth = 224;         ///< 输入宽度
    int m_inputHeight = 224;        ///< 输入高度
    QVector<float> m_mean;          ///< 归一化均值
    QVector<float> m_std;           ///< 归一化标准差
};

/**
 * @brief 图像分类模型推理类
 *
 * 支持加载 TorchScript 格式的图像分类模型（如 ResNet、EfficientNet 等）
 */
class ImageClassificationModel : public TorchModelInference
{
public:
    ImageClassificationModel();

    /**
     * @brief 执行图像分类推理
     * @param image 输入图像
     * @return 分类结果
     */
    ClassificationResult classify(const QImage &image);

    // 实现 ModelInference 接口
    InferenceResult infer(const QImage &image) override;

    /**
     * @brief 设置类别标签
     * @param labels 类别标签列表
     */
    void setClassLabels(const QStringList &labels);

    /**
     * @brief 设置 Top-K
     * @param k 返回前 K 个预测结果
     */
    void setTopK(int k);

protected:
    torch::Tensor preprocessImage(const QImage &image) override;
    InferenceResult postprocessOutput(const torch::Tensor &output) override;

private:
    QStringList m_classLabels;  ///< 类别标签
    int m_topK = 5;             ///< Top-K 值
};

#endif // LIBTORCH_FOUND

} // namespace Models
} // namespace GenPreCVSystem

#endif // MODELINFERENCE_H
