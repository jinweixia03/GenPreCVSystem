#include "imagecontroller.h"
#include <QImage>
#include <QTransform>

namespace GenPreCVSystem {
namespace Controllers {

ImageController::ImageController(QObject *parent)
    : QObject(parent)
    , m_undoStack(nullptr)
{
}

void ImageController::setCurrentImage(const QPixmap &pixmap, Models::UndoStack *undoStack)
{
    m_currentPixmap = pixmap;
    m_undoStack = undoStack;
}

void ImageController::rotateLeft()
{
    applyImageOperation(
        [](const QImage &img) { return Utils::ImageProcessor::rotate(img, -90); },
        "向左旋转90°"
    );
}

void ImageController::rotateRight()
{
    applyImageOperation(
        [](const QImage &img) { return Utils::ImageProcessor::rotate(img, 90); },
        "向右旋转90°"
    );
}

void ImageController::flipHorizontal()
{
    applyImageOperation(
        [](const QImage &img) { return Utils::ImageProcessor::flipHorizontal(img); },
        "水平翻转"
    );
}

void ImageController::flipVertical()
{
    applyImageOperation(
        [](const QImage &img) { return Utils::ImageProcessor::flipVertical(img); },
        "垂直翻转"
    );
}

void ImageController::toGrayscale()
{
    applyImageOperation(
        [](const QImage &img) { return Utils::ImageProcessor::toGrayscale(img); },
        "已转换为灰度图"
    );
}

void ImageController::invert()
{
    applyImageOperation(
        [](const QImage &img) { return Utils::ImageProcessor::invert(img); },
        "已反色"
    );
}

void ImageController::blur(int radius)
{
    applyImageOperation(
        [radius](const QImage &img) { return Utils::ImageProcessor::gaussianBlur(img, radius); },
        QString("模糊处理 (半径=%1)").arg(radius)
    );
}

void ImageController::sharpen(double strength)
{
    applyImageOperation(
        [strength](const QImage &img) { return Utils::ImageProcessor::sharpen(img, strength); },
        QString("锐化处理 (强度=%1)").arg(strength)
    );
}

void ImageController::edgeDetection()
{
    applyImageOperation(
        [](const QImage &img) { return Utils::ImageProcessor::sobelEdgeDetection(img); },
        "Sobel边缘检测"
    );
}

void ImageController::threshold(int value)
{
    applyImageOperation(
        [value](const QImage &img) { return Utils::ImageProcessor::threshold(img, value); },
        QString("二值化 (阈值=%1)").arg(value)
    );
}

void ImageController::applyImageOperation(const std::function<QImage(const QImage&)> &operation, const QString &operationName)
{
    if (!hasImage()) {
        emit operationFailed("请先打开图片");
        return;
    }

    // 保存状态到撤销栈
    if (m_undoStack) {
        m_undoStack->push(m_currentPixmap);
    }

    // 应用操作
    QImage image = m_currentPixmap.toImage();
    QImage result = operation(image);
    m_currentPixmap = QPixmap::fromImage(result);

    emit imageChanged(m_currentPixmap);
}

} // namespace Controllers
} // namespace GenPreCVSystem
