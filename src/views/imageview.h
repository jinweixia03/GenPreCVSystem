#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsPolygonItem>
#include <QPixmap>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QVector>
#include <QPointF>

namespace GenPreCVSystem {
namespace Views {

/**
 * @brief 检测结果结构（用于显示）
 */
struct DetectionOverlay {
    int x;
    int y;
    int width;
    int height;
    float confidence;
    QString label;
    QColor color;
    QVector<QPointF> maskPolygon;  // 分割掩码多边形
};

/**
 * @brief 图片显示视图
 *
 * 继承自 QGraphicsView，实现图片显示、缩放、平移等功能
 */
class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);

    /**
     * @brief 设置要显示的图片
     * @param pixmap 图片数据
     */
    void setPixmap(const QPixmap &pixmap);

    /**
     * @brief 清空显示的图片
     */
    void clearImage();

    /**
     * @brief 缩放图片
     * @param factor 缩放因子（>1 放大，<1 缩小）
     */
    void scaleImage(double factor);

    /**
     * @brief 自适应窗口大小显示图片
     */
    void fitToWindow();

    /**
     * @brief 获取当前缩放比例
     * @return 当前缩放比例
     */
    double currentScale() const { return m_scaleFactor; }

    /**
     * @brief 缩放至实际大小
     */
    void actualSize();

    /**
     * @brief 获取当前显示的图片
     */
    QPixmap pixmap() const;

    /**
     * @brief 设置检测结果覆盖层
     * @param detections 检测结果列表
     * @param showLabels 是否显示标签
     */
    void setDetections(const QVector<DetectionOverlay> &detections, bool showLabels = true);

    /**
     * @brief 设置分割结果覆盖层 (带掩码)
     * @param detections 检测结果列表
     * @param maskAlpha 掩码透明度 (0-100)
     * @param showBoxes 是否显示边界框
     * @param showLabels 是否显示标签
     */
    void setSegmentationOverlays(const QVector<DetectionOverlay> &detections,
                                  int maskAlpha = 50, bool showBoxes = false, bool showLabels = true);

    /**
     * @brief 清除检测结果覆盖层
     */
    void clearDetections();

signals:
    /**
     * @brief 图片缩放比例改变信号
     */
    void scaleChanged(double scale);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    double m_scaleFactor;
    bool m_dragging;
    QPoint m_lastPanPoint;

    // 检测结果覆盖层
    QVector<QGraphicsRectItem *> m_detectionBoxes;
    QVector<QGraphicsTextItem *> m_detectionLabels;
    QVector<QGraphicsPolygonItem *> m_maskPolygons;  // 分割掩码多边形
};

} // namespace Views
} // namespace GenPreCVSystem

#endif // IMAGEVIEW_H
