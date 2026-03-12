#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QImage>
#include <QVector>
#include <QPixmap>

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 图像处理工具类
 *
 * 提供各种图像处理算法：卷积、模糊、锐化、边缘检测等
 */
class ImageProcessor
{
public:
    /**
     * @brief 应用卷积核到图片
     * @param image 输入图片
     * @param kernel 卷积核（NxN）
     * @param divisor 除数
     * @param offset 偏移量
     * @return 处理后的图片
     */
    static QImage applyConvolution(const QImage &image,
                                   const QVector<QVector<int>> &kernel,
                                   int divisor = 1,
                                   int offset = 0);

    /**
     * @brief 高斯模糊
     * @param image 输入图片
     * @param radius 模糊半径
     * @return 模糊后的图片
     */
    static QImage gaussianBlur(const QImage &image, int radius = 2);

    /**
     * @brief 锐化图片
     * @param image 输入图片
     * @param strength 锐化强度
     * @return 锐化后的图片
     */
    static QImage sharpen(const QImage &image, double strength = 1.0);

    /**
     * @brief Sobel边缘检测
     * @param image 输入图片
     * @return 边缘检测后的图片
     */
    static QImage sobelEdgeDetection(const QImage &image);

    /**
     * @brief 转换为灰度图
     * @param image 输入图片
     * @return 灰度图
     */
    static QImage toGrayscale(const QImage &image);

    /**
     * @brief 反色
     * @param image 输入图片
     * @return 反色后的图片
     */
    static QImage invert(const QImage &image);

    /**
     * @brief 二值化
     * @param image 输入图片
     * @param threshold 阈值（0-255）
     * @return 二值化后的图片
     */
    static QImage threshold(const QImage &image, int threshold = 128);

    /**
     * @brief 旋转图片
     * @param image 输入图片
     * @param angle 旋转角度
     * @return 旋转后的图片
     */
    static QImage rotate(const QImage &image, qreal angle);

    /**
     * @brief 水平翻转
     * @param image 输入图片
     * @return 翻转后的图片
     */
    static QImage flipHorizontal(const QImage &image);

    /**
     * @brief 垂直翻转
     * @param image 输入图片
     * @return 翻转后的图片
     */
    static QImage flipVertical(const QImage &image);
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // IMAGEPROCESSOR_H
