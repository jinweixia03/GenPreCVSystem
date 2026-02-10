#include "imageview.h"
#include <QScrollBar>

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
        "ImageView { background-color: #1e1e1e; border: none; }"
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

} // namespace Views
} // namespace GenPreCVSystem
