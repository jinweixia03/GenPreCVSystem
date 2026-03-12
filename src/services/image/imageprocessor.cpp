#include "imageprocessor.h"
#include <QTransform>
#include <QtMath>

namespace GenPreCVSystem {
namespace Utils {

QImage ImageProcessor::applyConvolution(const QImage &image,
                                        const QVector<QVector<int>> &kernel,
                                        int divisor,
                                        int offset)
{
    if (image.isNull() || kernel.isEmpty() || kernel.size() % 2 == 0 || kernel[0].size() % 2 == 0) {
        return image;
    }

    int kernelSize = kernel.size();
    int half = kernelSize / 2;

    QImage result(image.size(), QImage::Format_RGB32);
    QImage src = image.convertToFormat(QImage::Format_RGB32);

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int r = 0, g = 0, b = 0;

            for (int ky = 0; ky < kernelSize; ++ky) {
                for (int kx = 0; kx < kernelSize; ++kx) {
                    int px = qBound(0, x + kx - half, image.width() - 1);
                    int py = qBound(0, y + ky - half, image.height() - 1);

                    QColor pixel = src.pixelColor(px, py);
                    int weight = kernel[ky][kx];

                    r += pixel.red() * weight;
                    g += pixel.green() * weight;
                    b += pixel.blue() * weight;
                }
            }

            r = qBound(0, r / divisor + offset, 255);
            g = qBound(0, g / divisor + offset, 255);
            b = qBound(0, b / divisor + offset, 255);

            result.setPixelColor(x, y, QColor(r, g, b));
        }
    }

    return result;
}

QImage ImageProcessor::gaussianBlur(const QImage &image, int radius)
{
    // 使用简单的盒式模糊作为近似
    QImage result = image;
    int iterations = radius * 2;

    for (int i = 0; i < iterations; ++i) {
        QVector<QVector<int>> kernel = {
            {1, 2, 1},
            {2, 4, 2},
            {1, 2, 1}
        };

        QImage temp(result.size(), QImage::Format_RGB32);
        for (int y = 0; y < result.height(); ++y) {
            for (int x = 0; x < result.width(); ++x) {
                int r = 0, g = 0, b = 0, count = 0;

                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int px = qBound(0, x + kx, result.width() - 1);
                        int py = qBound(0, y + ky, result.height() - 1);
                        QColor pixel = result.pixelColor(px, py);
                        int weight = kernel[ky + 1][kx + 1];

                        r += pixel.red() * weight;
                        g += pixel.green() * weight;
                        b += pixel.blue() * weight;
                        count += weight;
                    }
                }

                temp.setPixelColor(x, y, QColor(r / count, g / count, b / count));
            }
        }

        result = temp;
    }

    return result;
}

QImage ImageProcessor::sharpen(const QImage &image, double strength)
{
    // 锐化卷积核
    int s = static_cast<int>(strength);
    QVector<QVector<int>> kernel = {
        {0,        -s,       0},
        {-s,   1 + 4*s,    -s},
        {0,        -s,       0}
    };

    return applyConvolution(image, kernel, 1);
}

QImage ImageProcessor::sobelEdgeDetection(const QImage &image)
{
    if (image.isNull()) {
        return image;
    }

    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    QImage result(image.size(), QImage::Format_Grayscale8);

    // Sobel算子
    QVector<QVector<int>> sobelX = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };

    QVector<QVector<int>> sobelY = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    for (int y = 1; y < gray.height() - 1; ++y) {
        for (int x = 1; x < gray.width() - 1; ++x) {
            int gx = 0, gy = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int pixel = gray.pixelIndex(x + kx, y + ky);
                    gx += pixel * sobelX[ky + 1][kx + 1];
                    gy += pixel * sobelY[ky + 1][kx + 1];
                }
            }

            int magnitude = static_cast<int>(sqrt(gx * gx + gy * gy));
            result.setPixelColor(x, y, QColor(qBound(0, magnitude, 255),
                                              qBound(0, magnitude, 255),
                                              qBound(0, magnitude, 255)));
        }
    }

    return result;
}

QImage ImageProcessor::toGrayscale(const QImage &image)
{
    QImage result = image.convertToFormat(QImage::Format_RGB32);

    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            QRgb pixel = result.pixel(x, y);
            int gray = qGray(pixel);
            result.setPixel(x, y, qRgb(gray, gray, gray));
        }
    }

    return result;
}

QImage ImageProcessor::invert(const QImage &image)
{
    QImage result = image;
    result.invertPixels();
    return result;
}

QImage ImageProcessor::threshold(const QImage &image, int threshold)
{
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);

    for (int y = 0; y < gray.height(); ++y) {
        for (int x = 0; x < gray.width(); ++x) {
            uint8_t pixel = gray.bits()[y * gray.bytesPerLine() + x];
            gray.bits()[y * gray.bytesPerLine() + x] = (pixel >= threshold) ? 255 : 0;
        }
    }

    return gray;
}

QImage ImageProcessor::rotate(const QImage &image, qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return image.transformed(transform);
}

QImage ImageProcessor::flipHorizontal(const QImage &image)
{
    return image.transformed(QTransform().scale(-1, 1));
}

QImage ImageProcessor::flipVertical(const QImage &image)
{
    return image.transformed(QTransform().scale(1, -1));
}

} // namespace Utils
} // namespace GenPreCVSystem
