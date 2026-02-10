#ifndef IMAGECONTROLLER_H
#define IMAGECONTROLLER_H

#include <QObject>
#include <QPixmap>

#include "../models/undostack.h"
#include "../utils/imageprocessor.h"

namespace GenPreCVSystem {
namespace Controllers {

/**
 * @brief 图像处理控制器
 *
 * 管理图像的编辑操作，包括滤镜、变换等
 */
class ImageController : public QObject
{
    Q_OBJECT

public:
    explicit ImageController(QObject *parent = nullptr);

    /**
     * @brief 设置当前图片和撤销栈
     */
    void setCurrentImage(const QPixmap &pixmap, Models::UndoStack *undoStack);

    /**
     * @brief 获取当前处理后的图片
     */
    QPixmap currentImage() const { return m_currentPixmap; }

    // 图像变换操作
    void rotateLeft();
    void rotateRight();
    void flipHorizontal();
    void flipVertical();

    // 图像滤镜操作
    void toGrayscale();
    void invert();
    void blur(int radius = 3);
    void sharpen(double strength = 1.0);
    void edgeDetection();
    void threshold(int value = 128);

    /**
     * @brief 检查是否有当前图片
     */
    bool hasImage() const { return !m_currentPixmap.isNull(); }

signals:
    /**
     * @brief 图片已改变信号
     */
    void imageChanged(const QPixmap &newPixmap);

    /**
     * @brief 操作失败信号
     */
    void operationFailed(const QString &message);

private:
    void applyImageOperation(const std::function<QImage(const QImage&)> &operation, const QString &operationName);

    QPixmap m_currentPixmap;
    Models::UndoStack *m_undoStack;
};

} // namespace Controllers
} // namespace GenPreCVSystem

#endif // IMAGECONTROLLER_H
