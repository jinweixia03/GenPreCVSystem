#include "imageprocessservice.h"
#include <QElapsedTimer>
#include <QtMath>
#include <QDebug>

namespace GenPreCVSystem {
namespace Utils {

ImageProcessService::ImageProcessService(QObject *parent)
    : QObject(parent)
{
}

ImageProcessService::~ImageProcessService()
{
}

ProcessResult ImageProcessService::enhanceImage(const QImage &image,
                                                 int brightness,
                                                 int contrast,
                                                 int saturation,
                                                 int sharpness)
{
    ProcessResult result;
    QElapsedTimer timer;
    timer.start();

    if (image.isNull()) {
        result.success = false;
        result.message = "输入图像为空";
        return result;
    }

    emit logMessage(QString("图像增强: 亮度=%1, 对比度=%2, 饱和度=%3, 锐化=%4")
                    .arg(brightness).arg(contrast).arg(saturation).arg(sharpness));

    QImage processed = image.convertToFormat(QImage::Format_ARGB32);

    // 应用亮度
    if (brightness != 0) {
        processed = applyBrightness(processed, brightness);
    }

    // 应用对比度
    if (contrast != 0) {
        processed = applyContrast(processed, contrast);
    }

    // 应用饱和度
    if (saturation != 0) {
        processed = applySaturation(processed, saturation);
    }

    // 应用锐化
    if (sharpness > 0) {
        processed = applySharpness(processed, sharpness);
    }

    result.success = true;
    result.processedImage = processed;
    result.processTime = timer.elapsed();
    result.message = QString("图像增强完成，耗时 %1ms").arg(result.processTime);

    emit logMessage(result.message);
    return result;
}

ProcessResult ImageProcessService::denoiseImage(const QImage &image,
                                                 DenoiseMethod method,
                                                 int kernelSize,
                                                 double sigma)
{
    ProcessResult result;
    QElapsedTimer timer;
    timer.start();

    if (image.isNull()) {
        result.success = false;
        result.message = "输入图像为空";
        return result;
    }

    QString methodName;
    switch (method) {
        case DenoiseMethod::Gaussian: methodName = "高斯滤波"; break;
        case DenoiseMethod::Bilateral: methodName = "双边滤波"; break;
        case DenoiseMethod::Median: methodName = "中值滤波"; break;
        case DenoiseMethod::NLM: methodName = "非局部均值"; break;
    }

    emit logMessage(QString("图像去噪: 方法=%1, 卷积核=%2, sigma=%3")
                    .arg(methodName).arg(kernelSize).arg(sigma));

    QImage processed = image.convertToFormat(QImage::Format_ARGB32);

    switch (method) {
        case DenoiseMethod::Gaussian:
            processed = applyGaussianBlur(processed, kernelSize, sigma);
            break;
        case DenoiseMethod::Bilateral:
            processed = applyBilateralFilter(processed, kernelSize, sigma);
            break;
        case DenoiseMethod::Median:
            processed = applyMedianFilter(processed, kernelSize);
            break;
        case DenoiseMethod::NLM:
            // NLM 比较复杂，暂时用高斯代替
            processed = applyGaussianBlur(processed, kernelSize, sigma);
            break;
    }

    result.success = true;
    result.processedImage = processed;
    result.processTime = timer.elapsed();
    result.message = QString("图像去噪完成，耗时 %1ms").arg(result.processTime);

    emit logMessage(result.message);
    return result;
}

ProcessResult ImageProcessService::detectEdges(const QImage &image,
                                                EdgeMethod method,
                                                double threshold1,
                                                double threshold2,
                                                int apertureSize)
{
    ProcessResult result;
    QElapsedTimer timer;
    timer.start();

    if (image.isNull()) {
        result.success = false;
        result.message = "输入图像为空";
        return result;
    }

    QString methodName;
    switch (method) {
        case EdgeMethod::Sobel: methodName = "Sobel"; break;
        case EdgeMethod::Canny: methodName = "Canny"; break;
        case EdgeMethod::Laplacian: methodName = "Laplacian"; break;
        case EdgeMethod::Scharr: methodName = "Scharr"; break;
    }

    emit logMessage(QString("边缘检测: 方法=%1, 阈值1=%2, 阈值2=%3")
                    .arg(methodName).arg(threshold1).arg(threshold2));

    QImage processed;

    switch (method) {
        case EdgeMethod::Sobel:
            processed = applySobel(image, apertureSize);
            break;
        case EdgeMethod::Canny:
            processed = applyCanny(image, threshold1, threshold2, apertureSize);
            break;
        case EdgeMethod::Laplacian:
            processed = applyLaplacian(image, apertureSize);
            break;
        case EdgeMethod::Scharr:
            // Scharr 是 Sobel 的变体，使用固定卷积核大小3
            processed = applySobel(image, 3);
            break;
    }

    result.success = true;
    result.processedImage = processed;
    result.processTime = timer.elapsed();
    result.message = QString("边缘检测完成，耗时 %1ms").arg(result.processTime);

    emit logMessage(result.message);
    return result;
}

// 辅助函数实现

QImage ImageProcessService::applyBrightness(const QImage &image, int brightness)
{
    QImage result(image.size(), QImage::Format_ARGB32);

    for (int y = 0; y < image.height(); ++y) {
        const QRgb *srcLine = reinterpret_cast<const QRgb*>(image.scanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < image.width(); ++x) {
            int r = qRed(srcLine[x]) + brightness;
            int g = qGreen(srcLine[x]) + brightness;
            int b = qBlue(srcLine[x]) + brightness;
            int a = qAlpha(srcLine[x]);

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    return result;
}

QImage ImageProcessService::applyContrast(const QImage &image, int contrast)
{
    QImage result(image.size(), QImage::Format_ARGB32);
    double factor = (259.0 * (contrast + 255)) / (255.0 * (259 - contrast));

    for (int y = 0; y < image.height(); ++y) {
        const QRgb *srcLine = reinterpret_cast<const QRgb*>(image.scanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < image.width(); ++x) {
            int r = static_cast<int>(factor * (qRed(srcLine[x]) - 128) + 128);
            int g = static_cast<int>(factor * (qGreen(srcLine[x]) - 128) + 128);
            int b = static_cast<int>(factor * (qBlue(srcLine[x]) - 128) + 128);
            int a = qAlpha(srcLine[x]);

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    return result;
}

QImage ImageProcessService::applySaturation(const QImage &image, int saturation)
{
    QImage result(image.size(), QImage::Format_ARGB32);
    double satFactor = 1.0 + saturation / 100.0;

    for (int y = 0; y < image.height(); ++y) {
        const QRgb *srcLine = reinterpret_cast<const QRgb*>(image.scanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < image.width(); ++x) {
            int r = qRed(srcLine[x]);
            int g = qGreen(srcLine[x]);
            int b = qBlue(srcLine[x]);
            int a = qAlpha(srcLine[x]);

            // 转换到 YUV 色彩空间调整饱和度
            double gray = 0.299 * r + 0.587 * g + 0.114 * b;

            r = static_cast<int>(gray + satFactor * (r - gray));
            g = static_cast<int>(gray + satFactor * (g - gray));
            b = static_cast<int>(gray + satFactor * (b - gray));

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            dstLine[x] = qRgba(r, g, b, a);
        }
    }

    return result;
}

QImage ImageProcessService::applySharpness(const QImage &image, int sharpness)
{
    QImage result(image.size(), QImage::Format_ARGB32);
    double factor = sharpness / 100.0;

    // 锐化卷积核
    double kernel[3][3] = {
        {0, -factor, 0},
        {-factor, 1 + 4 * factor, -factor},
        {0, -factor, 0}
    };

    for (int y = 1; y < image.height() - 1; ++y) {
        const QRgb *srcPrev = reinterpret_cast<const QRgb*>(image.scanLine(y - 1));
        const QRgb *srcCurr = reinterpret_cast<const QRgb*>(image.scanLine(y));
        const QRgb *srcNext = reinterpret_cast<const QRgb*>(image.scanLine(y + 1));
        QRgb *dstLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 1; x < image.width() - 1; ++x) {
            double r = 0, g = 0, b = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    const QRgb *line;
                    if (ky == -1) line = srcPrev;
                    else if (ky == 0) line = srcCurr;
                    else line = srcNext;

                    QRgb pixel = line[x + kx];
                    double k = kernel[ky + 1][kx + 1];

                    r += qRed(pixel) * k;
                    g += qGreen(pixel) * k;
                    b += qBlue(pixel) * k;
                }
            }

            dstLine[x] = qRgba(qBound(0, static_cast<int>(r), 255),
                               qBound(0, static_cast<int>(g), 255),
                               qBound(0, static_cast<int>(b), 255),
                               qAlpha(srcCurr[x]));
        }
    }

    // 复制边缘
    for (int x = 0; x < image.width(); ++x) {
        result.setPixel(x, 0, image.pixel(x, 0));
        result.setPixel(x, image.height() - 1, image.pixel(x, image.height() - 1));
    }
    for (int y = 0; y < image.height(); ++y) {
        result.setPixel(0, y, image.pixel(0, y));
        result.setPixel(image.width() - 1, y, image.pixel(image.width() - 1, y));
    }

    return result;
}

QImage ImageProcessService::applyGaussianBlur(const QImage &image, int kernelSize, double sigma)
{
    if (kernelSize < 3) kernelSize = 3;
    if (kernelSize % 2 == 0) kernelSize++;

    QImage result(image.size(), QImage::Format_ARGB32);
    int half = kernelSize / 2;

    // 创建高斯核
    QVector<QVector<double>> kernel(kernelSize, QVector<double>(kernelSize));
    double sum = 0;

    for (int y = 0; y < kernelSize; ++y) {
        for (int x = 0; x < kernelSize; ++x) {
            int dx = x - half;
            int dy = y - half;
            kernel[y][x] = qExp(-(dx * dx + dy * dy) / (2 * sigma * sigma));
            sum += kernel[y][x];
        }
    }

    // 归一化
    for (int y = 0; y < kernelSize; ++y) {
        for (int x = 0; x < kernelSize; ++x) {
            kernel[y][x] /= sum;
        }
    }

    // 应用卷积
    QImage temp = image.convertToFormat(QImage::Format_ARGB32);

    for (int y = 0; y < image.height(); ++y) {
        QRgb *dstLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < image.width(); ++x) {
            double r = 0, g = 0, b = 0, a = 0;

            for (int ky = 0; ky < kernelSize; ++ky) {
                int sy = qBound(0, y + ky - half, image.height() - 1);
                const QRgb *srcLine = reinterpret_cast<const QRgb*>(temp.scanLine(sy));

                for (int kx = 0; kx < kernelSize; ++kx) {
                    int sx = qBound(0, x + kx - half, image.width() - 1);
                    QRgb pixel = srcLine[sx];
                    double k = kernel[ky][kx];

                    r += qRed(pixel) * k;
                    g += qGreen(pixel) * k;
                    b += qBlue(pixel) * k;
                    a += qAlpha(pixel) * k;
                }
            }

            dstLine[x] = qRgba(static_cast<int>(r), static_cast<int>(g),
                               static_cast<int>(b), static_cast<int>(a));
        }
    }

    return result;
}

QImage ImageProcessService::applyBilateralFilter(const QImage &image, int kernelSize, double sigma)
{
    // 简化的双边滤波实现
    if (kernelSize < 3) kernelSize = 3;
    if (kernelSize % 2 == 0) kernelSize++;

    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    // 双边滤波比较复杂，这里用高斯模糊近似
    return applyGaussianBlur(result, kernelSize, sigma);
}

QImage ImageProcessService::applyMedianFilter(const QImage &image, int kernelSize)
{
    if (kernelSize < 3) kernelSize = 3;
    if (kernelSize % 2 == 0) kernelSize++;

    QImage result(image.size(), QImage::Format_ARGB32);
    int half = kernelSize / 2;

    QImage temp = image.convertToFormat(QImage::Format_ARGB32);

    for (int y = 0; y < image.height(); ++y) {
        QRgb *dstLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < image.width(); ++x) {
            QVector<int> reds, greens, blues;

            for (int ky = -half; ky <= half; ++ky) {
                int sy = qBound(0, y + ky, image.height() - 1);
                const QRgb *srcLine = reinterpret_cast<const QRgb*>(temp.scanLine(sy));

                for (int kx = -half; kx <= half; ++kx) {
                    int sx = qBound(0, x + kx, image.width() - 1);
                    QRgb pixel = srcLine[sx];
                    reds.append(qRed(pixel));
                    greens.append(qGreen(pixel));
                    blues.append(qBlue(pixel));
                }
            }

            std::sort(reds.begin(), reds.end());
            std::sort(greens.begin(), greens.end());
            std::sort(blues.begin(), blues.end());

            int mid = reds.size() / 2;
            dstLine[x] = qRgb(reds[mid], greens[mid], blues[mid]);
        }
    }

    return result;
}

QImage ImageProcessService::toGrayscale(const QImage &image)
{
    QImage result(image.size(), QImage::Format_Grayscale8);

    for (int y = 0; y < image.height(); ++y) {
        const QRgb *srcLine = reinterpret_cast<const QRgb*>(image.scanLine(y));
        uchar *dstLine = result.scanLine(y);

        for (int x = 0; x < image.width(); ++x) {
            dstLine[x] = static_cast<uchar>(qGray(srcLine[x]));
        }
    }

    return result;
}

QImage ImageProcessService::applySobel(const QImage &image, int ksize)
{
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    QImage result(image.size(), QImage::Format_Grayscale8);

    // 初始化整个结果图像为黑色
    result.fill(0);

    // Sobel kernels
    int sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int sobelY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    for (int y = 1; y < gray.height() - 1; ++y) {
        const uchar *prevLine = gray.scanLine(y - 1);
        const uchar *currLine = gray.scanLine(y);
        const uchar *nextLine = gray.scanLine(y + 1);
        uchar *dstLine = result.scanLine(y);

        for (int x = 1; x < gray.width() - 1; ++x) {
            int gx = 0, gy = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                const uchar *line;
                if (ky == -1) line = prevLine;
                else if (ky == 0) line = currLine;
                else line = nextLine;

                for (int kx = -1; kx <= 1; ++kx) {
                    int pixel = line[x + kx];
                    gx += pixel * sobelX[ky + 1][kx + 1];
                    gy += pixel * sobelY[ky + 1][kx + 1];
                }
            }

            int magnitude = static_cast<int>(qSqrt(gx * gx + gy * gy));
            dstLine[x] = qBound(0, magnitude, 255);
        }
    }

    return result;
}

QImage ImageProcessService::applyCanny(const QImage &image, double threshold1, double threshold2, int apertureSize)
{
    // 简化的 Canny 实现：先 Sobel，然后阈值处理
    QImage sobelResult = applySobel(image, apertureSize);

    QImage result(sobelResult.size(), QImage::Format_Grayscale8);

    // 初始化整个结果图像为黑色
    result.fill(0);

    for (int y = 0; y < sobelResult.height(); ++y) {
        const uchar *srcLine = sobelResult.scanLine(y);
        uchar *dstLine = result.scanLine(y);

        for (int x = 0; x < sobelResult.width(); ++x) {
            int val = srcLine[x];

            if (val >= threshold2) {
                dstLine[x] = 255;  // 强边缘
            } else if (val >= threshold1) {
                dstLine[x] = 128;  // 弱边缘
            } else {
                dstLine[x] = 0;    // 非边缘
            }
        }
    }

    return result;
}

QImage ImageProcessService::applyLaplacian(const QImage &image, int ksize)
{
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    QImage result(image.size(), QImage::Format_Grayscale8);

    // 初始化整个结果图像为黑色
    result.fill(0);

    // Laplacian kernel
    int laplacian[3][3] = {{0, 1, 0}, {1, -4, 1}, {0, 1, 0}};

    for (int y = 1; y < gray.height() - 1; ++y) {
        const uchar *prevLine = gray.scanLine(y - 1);
        const uchar *currLine = gray.scanLine(y);
        const uchar *nextLine = gray.scanLine(y + 1);
        uchar *dstLine = result.scanLine(y);

        for (int x = 1; x < gray.width() - 1; ++x) {
            int sum = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                const uchar *line;
                if (ky == -1) line = prevLine;
                else if (ky == 0) line = currLine;
                else line = nextLine;

                for (int kx = -1; kx <= 1; ++kx) {
                    sum += line[x + kx] * laplacian[ky + 1][kx + 1];
                }
            }

            dstLine[x] = qBound(0, qAbs(sum), 255);
        }
    }

    return result;
}

} // namespace Utils
} // namespace GenPreCVSystem
