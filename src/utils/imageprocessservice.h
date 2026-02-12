#ifndef IMAGEPROCESSSERVICE_H
#define IMAGEPROCESSSERVICE_H

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QString>

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 图像处理结果
 */
struct ProcessResult {
    bool success = false;
    QString message;
    QImage processedImage;
    double processTime = 0.0;
};

/**
 * @brief 图像处理服务
 *
 * 提供图像增强、去噪、边缘检测等传统图像处理功能
 */
class ImageProcessService : public QObject
{
    Q_OBJECT

public:
    explicit ImageProcessService(QObject *parent = nullptr);
    ~ImageProcessService();

    // 图像增强
    ProcessResult enhanceImage(const QImage &image,
                                int brightness,
                                int contrast,
                                int saturation,
                                int sharpness);

    // 图像去噪
    enum class DenoiseMethod {
        Gaussian,
        Bilateral,
        Median,
        NLM
    };
    ProcessResult denoiseImage(const QImage &image,
                                DenoiseMethod method,
                                int kernelSize,
                                double sigma);

    // 边缘检测
    enum class EdgeMethod {
        Sobel,
        Canny,
        Laplacian,
        Scharr
    };
    ProcessResult detectEdges(const QImage &image,
                               EdgeMethod method,
                               double threshold1,
                               double threshold2,
                               int apertureSize);

    // 图像分类（使用 YOLO 服务）
    // 关键点检测（使用 YOLO-pose 服务）
    // 语义分割（使用 YOLO-seg 服务）

signals:
    void processCompleted(const ProcessResult &result);
    void logMessage(const QString &message);

private:
    // 辅助函数
    QImage applyBrightness(const QImage &image, int brightness);
    QImage applyContrast(const QImage &image, int contrast);
    QImage applySaturation(const QImage &image, int saturation);
    QImage applySharpness(const QImage &image, int sharpness);
    QImage applyGaussianBlur(const QImage &image, int kernelSize, double sigma);
    QImage applyBilateralFilter(const QImage &image, int kernelSize, double sigma);
    QImage applyMedianFilter(const QImage &image, int kernelSize);
    QImage applySobel(const QImage &image, int ksize);
    QImage applyCanny(const QImage &image, double threshold1, double threshold2, int apertureSize);
    QImage applyLaplacian(const QImage &image, int ksize);
    QImage toGrayscale(const QImage &image);
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // IMAGEPROCESSSERVICE_H
