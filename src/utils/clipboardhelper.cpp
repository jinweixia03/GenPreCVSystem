#include "clipboardhelper.h"
#include <QFileInfo>
#include <QDateTime>
#include <QVariant>

namespace GenPreCVSystem {
namespace Utils {

void ClipboardHelper::copyImage(const QPixmap &pixmap, const QString &filePath)
{
    if (pixmap.isNull()) {
        return;
    }

    // 创建 QMimeData 同时支持两种格式
    QMimeData *mimeData = new QMimeData();

    // 1. 设置图片数据 - 用于在图片编辑器、Word等应用中粘贴图片内容
    mimeData->setImageData(pixmap.toImage());

    // 2. 如果有文件路径，同时设置文件引用 - 用于在文件管理器中粘贴文件副本
    if (!filePath.isEmpty()) {
        QList<QUrl> urls;
        urls << QUrl::fromLocalFile(filePath);
        mimeData->setUrls(urls);
    }

    QApplication::clipboard()->setMimeData(mimeData);
}

bool ClipboardHelper::pasteImage(QPixmap &outPixmap, QString &outFileName)
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData) {
        return false;
    }

    outPixmap = QPixmap();
    outFileName.clear();

    // 优先从 URL 获取文件（保留原文件名）
    if (mimeData->hasUrls()) {
        if (getImageFromUrls(mimeData->urls(), outPixmap, outFileName)) {
            return true;
        }
    }

    // 如果没有从 URL 获取到图片，尝试直接获取图片数据
    if (mimeData->hasImage()) {
        return getImageFromData(mimeData, outPixmap, outFileName);
    }

    return false;
}

bool ClipboardHelper::hasImage()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData) {
        return false;
    }

    return mimeData->hasImage() || mimeData->hasUrls();
}

bool ClipboardHelper::getImageFromUrls(const QList<QUrl> &urls, QPixmap &outPixmap, QString &outFileName)
{
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);

            // 检查是否是图片文件
            QPixmap pixmap(filePath);
            if (!pixmap.isNull()) {
                outPixmap = pixmap;
                outFileName = fileInfo.fileName();
                return true;
            }
        }
    }

    return false;
}

bool ClipboardHelper::getImageFromData(const QMimeData *mimeData, QPixmap &outPixmap, QString &outFileName)
{
    QImage image = qvariant_cast<QImage>(mimeData->imageData());
    if (!image.isNull()) {
        outPixmap = QPixmap::fromImage(image);
        // 使用时间戳生成文件名
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        outFileName = QString("pasted_%1.png").arg(timestamp);
        return true;
    }

    return false;
}

} // namespace Utils
} // namespace GenPreCVSystem
