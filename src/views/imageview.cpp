/**
 * @file imageview.cpp
 * @brief 图像显示视图实现
 *
 * 基于 QGraphicsView 的图像显示组件，支持：
 * - 图像缩放（鼠标滚轮）
 * - 图像平移（鼠标拖拽）
 * - 自适应窗口显示
 * - 检测结果可视化（边界框、标签、掩码）
 */

#include "imageview.h"
#include <QScrollBar>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QTextDocument>

namespace GenPreCVSystem {
namespace Views {

ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_pixmapItem(nullptr)
    , m_scaleFactor(1.0)
    , m_dragging(false)
{
    // 创建图形场景
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // 设置外观样式
    setStyleSheet(
        "ImageView { background-color: #ffffff; border: none; }"
    );

    // 设置渲染选项（抗锯齿、平滑）
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 设置拖拽模式为滚动拖拽
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    // 隐藏滚动条（使用鼠标拖拽平移）
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 设置缓存背景
    setCacheMode(QGraphicsView::CacheBackground);
}

void ImageView::setPixmap(const QPixmap &pixmap)
{
    // 清空场景
    m_scene->clear();
    m_pixmapItem = nullptr;

    if (pixmap.isNull()) {
        return;
    }

    // 创建图片项并添加到场景
    m_pixmapItem = m_scene->addPixmap(pixmap);
    m_scene->setSceneRect(pixmap.rect());

    // 自适应窗口大小
    fitToWindow();
}

void ImageView::clearImage()
{
    clearDetections();
    m_scene->clear();
    m_pixmapItem = nullptr;
    m_scaleFactor = 1.0;
    resetTransform();
}

void ImageView::scaleImage(double factor)
{
    if (!m_pixmapItem) {
        return;
    }

    // 限制缩放范围（0.1x ~ 10x）
    double newScale = m_scaleFactor * factor;
    if (newScale < 0.1 || newScale > 10.0) {
        return;
    }

    scale(factor, factor);
    m_scaleFactor = newScale;

    emit scaleChanged(m_scaleFactor);
}

void ImageView::fitToWindow()
{
    if (!m_pixmapItem) {
        return;
    }

    // 获取视图和图片尺寸
    QSize viewSize = viewport()->size();
    QSize imageSize = m_pixmapItem->pixmap().size();

    if (imageSize.isEmpty()) {
        return;
    }

    // 计算缩放比例
    double scaleX = static_cast<double>(viewSize.width()) / imageSize.width();
    double scaleY = static_cast<double>(viewSize.height()) / imageSize.height();
    double scaleFactor = qMin(scaleX, scaleY) * 0.95;  // 留5%边距

    // 重置变换并应用新缩放
    resetTransform();
    QGraphicsView::scale(scaleFactor, scaleFactor);
    m_scaleFactor = scaleFactor;

    // 居中显示
    centerOn(m_pixmapItem);

    emit scaleChanged(m_scaleFactor);
}

void ImageView::actualSize()
{
    if (!m_pixmapItem) {
        return;
    }

    resetTransform();
    m_scaleFactor = 1.0;
    centerOn(m_pixmapItem);

    emit scaleChanged(m_scaleFactor);
}

QPixmap ImageView::pixmap() const
{
    if (m_pixmapItem) {
        return m_pixmapItem->pixmap();
    }
    return QPixmap();
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if (!m_pixmapItem) {
        return;
    }

    // 获取滚轮滚动的角度
    QPoint angleDelta = event->angleDelta();
    double delta = angleDelta.y();

    // 计算缩放因子（每次滚轮滚动缩放10%）
    double factor = (delta > 0) ? 1.1 : 0.9;

    // 缩放图片
    scaleImage(factor);
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && m_pixmapItem) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();

        // 平移视图
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }
    QGraphicsView::mouseMoveEvent(event);
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    // 窗口大小改变时重新适应窗口
    if (m_pixmapItem) {
        fitToWindow();
    }
}

void ImageView::setDetections(const QVector<DetectionOverlay> &detections, bool showLabels)
{
    // 先清除现有的检测结果
    clearDetections();

    if (!m_scene || detections.isEmpty()) {
        return;
    }

    // 预定义的颜色列表（用于不同类别）
    QVector<QColor> colors = {
        QColor(255, 0, 0, 200),     // 红
        QColor(0, 255, 0, 200),     // 绿
        QColor(0, 0, 255, 200),     // 蓝
        QColor(255, 255, 0, 200),   // 黄
        QColor(255, 0, 255, 200),   // 洋红
        QColor(0, 255, 255, 200),   // 青
        QColor(255, 128, 0, 200),   // 橙
        QColor(128, 0, 255, 200),   // 紫
    };

    QFont labelFont("Arial", 10, QFont::Bold);

    for (int i = 0; i < detections.size(); ++i) {
        const DetectionOverlay &det = detections[i];

        // 选择颜色
        QColor color = det.color.isValid() ? det.color : colors[i % colors.size()];

        // 创建边界框
        QGraphicsRectItem *rectItem = m_scene->addRect(
            det.x, det.y, det.width, det.height,
            QPen(color, 2),
            QBrush(Qt::NoBrush)
        );
        rectItem->setZValue(1);  // 确保在图片上方
        m_detectionBoxes.append(rectItem);

        // 如果需要显示标签
        if (showLabels) {
            // 创建标签文本
            QString labelText = QString("%1 (%2%)")
                .arg(det.label)
                .arg(static_cast<int>(det.confidence * 100));

            QGraphicsTextItem *textItem = m_scene->addText(labelText, labelFont);
            textItem->setDefaultTextColor(Qt::white);
            textItem->setZValue(2);

            // 设置标签背景
            QTextDocument *doc = textItem->document();
            doc->setTextWidth(textItem->boundingRect().width());
            textItem->document()->setDefaultStyleSheet(
                "body { background-color: rgba(0,0,0,0.6); padding: 2px; }"
            );

            // 定位标签（在边界框上方）
            qreal textX = det.x;
            qreal textY = det.y - textItem->boundingRect().height();
            if (textY < 0) {
                textY = det.y;  // 如果超出上方，放在框内
            }
            textItem->setPos(textX, textY);
            m_detectionLabels.append(textItem);
        }
    }
}

void ImageView::clearDetections()
{
    // 删除所有边界框
    for (QGraphicsRectItem *item : m_detectionBoxes) {
        m_scene->removeItem(item);
        delete item;
    }
    m_detectionBoxes.clear();

    // 删除所有标签
    for (QGraphicsTextItem *item : m_detectionLabels) {
        m_scene->removeItem(item);
        delete item;
    }
    m_detectionLabels.clear();

    // 删除所有掩码多边形
    for (QGraphicsPolygonItem *item : m_maskPolygons) {
        m_scene->removeItem(item);
        delete item;
    }
    m_maskPolygons.clear();
}

void ImageView::setSegmentationOverlays(const QVector<DetectionOverlay> &detections,
                                         int maskAlpha, bool showBoxes, bool showLabels)
{
    // 先清除现有的检测结果
    clearDetections();

    if (!m_scene || detections.isEmpty()) {
        return;
    }

    // 预定义的颜色列表（用于不同类别）
    QVector<QColor> colors = {
        QColor(255, 0, 0),     // 红
        QColor(0, 255, 0),     // 绿
        QColor(0, 0, 255),     // 蓝
        QColor(255, 255, 0),   // 黄
        QColor(255, 0, 255),   // 洋红
        QColor(0, 255, 255),   // 青
        QColor(255, 128, 0),   // 橙
        QColor(128, 0, 255),   // 紫
    };

    QFont labelFont("Arial", 10, QFont::Bold);

    for (int i = 0; i < detections.size(); ++i) {
        const DetectionOverlay &det = detections[i];

        // 选择颜色
        QColor color = det.color.isValid() ? det.color : colors[i % colors.size()];

        // 绘制掩码多边形（如果有）
        if (!det.maskPolygon.isEmpty()) {
            QPolygonF polygon;
            for (const QPointF &pt : det.maskPolygon) {
                polygon << pt;
            }

            // 计算带透明度的填充颜色
            QColor fillColor = color;
            fillColor.setAlpha(static_cast<int>(maskAlpha * 2.55));  // 0-100 转换为 0-255

            QGraphicsPolygonItem *polyItem = m_scene->addPolygon(
                polygon,
                QPen(color, 2),
                QBrush(fillColor)
            );
            polyItem->setZValue(0.5);  // 在图片上方，但在边界框下方
            m_maskPolygons.append(polyItem);
        }

        // 如果需要显示边界框
        if (showBoxes) {
            QGraphicsRectItem *rectItem = m_scene->addRect(
                det.x, det.y, det.width, det.height,
                QPen(color, 2),
                QBrush(Qt::NoBrush)
            );
            rectItem->setZValue(1);
            m_detectionBoxes.append(rectItem);
        }

        // 如果需要显示标签
        if (showLabels) {
            QString labelText = QString("%1 (%2%)")
                .arg(det.label)
                .arg(static_cast<int>(det.confidence * 100));

            QGraphicsTextItem *textItem = m_scene->addText(labelText, labelFont);
            textItem->setDefaultTextColor(Qt::white);
            textItem->setZValue(2);

            // 设置标签背景
            QTextDocument *doc = textItem->document();
            doc->setTextWidth(textItem->boundingRect().width());
            textItem->document()->setDefaultStyleSheet(
                "body { background-color: rgba(0,0,0,0.6); padding: 2px; }"
            );

            // 定位标签
            qreal textX = det.x;
            qreal textY = det.y - textItem->boundingRect().height();
            if (textY < 0) {
                textY = det.y;
            }
            textItem->setPos(textX, textY);
            m_detectionLabels.append(textItem);
        }
    }
}

} // namespace Views
} // namespace GenPreCVSystem
