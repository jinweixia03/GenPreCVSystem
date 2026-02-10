#include "filetreeview.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QPixmap>
#include <QBuffer>
#include <QDateTime>
#include <QDir>

namespace GenPreCVSystem {
namespace Views {

FileTreeView::FileTreeView(QWidget *parent)
    : QTreeView(parent)
{
    // 启用拖放
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
}

void FileTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

void FileTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragMoveEvent(event);
    }
}

void FileTreeView::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    // 处理文件URL
    if (mimeData->hasUrls()) {
        QStringList imageFiles;
        QString folderPath;
        QList<QUrl> urls = mimeData->urls();

        for (const QUrl &url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);

                if (fileInfo.isDir()) {
                    // 如果是文件夹，记录文件夹路径
                    folderPath = filePath;
                    break;  // 只处理第一个文件夹
                }

                // 检查是否是图片文件
                if (!QPixmap(filePath).isNull()) {
                    imageFiles.append(filePath);
                }
            }
        }

        // 优先处理文件夹（打开文件夹）
        if (!folderPath.isEmpty()) {
            emit folderDropped(folderPath);
            event->acceptProposedAction();
            return;
        }

        // 处理图片文件
        if (!imageFiles.isEmpty()) {
            emit imagesDropped(imageFiles);
            event->acceptProposedAction();
            return;
        }
    }

    // 处理图片数据
    if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");

            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            QString sourceName = QString("dropped_%1.png").arg(timestamp);

            emit imageDataDropped(byteArray, sourceName);
            event->acceptProposedAction();
            return;
        }
    }

    QTreeView::dropEvent(event);
}

} // namespace Views
} // namespace GenPreCVSystem
