#include "imagefilefilterproxy.h"
#include <QFileSystemModel>
#include <QFileInfo>

namespace GenPreCVSystem {
namespace Views {

ImageFileFilterProxyModel::ImageFileFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool ImageFileFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QFileSystemModel *fileModel = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fileModel) {
        return true;
    }

    QModelIndex index = fileModel->index(sourceRow, 0, sourceParent);
    if (!index.isValid()) {
        return false;
    }

    // 始终显示目录
    if (fileModel->isDir(index)) {
        return true;
    }

    // 对于文件，只显示图片格式
    QString fileName = fileModel->fileName(index);
    QString suffix = QFileInfo(fileName).suffix().toLower();

    QStringList imageSuffixes = {
        "png", "jpg", "jpeg", "bmp", "gif", "tiff", "tif", "webp",
        "ico", "svg", "ppm", "pgm", "pbm", "pnm", "xbm", "xpm",
        "jp2", "j2k", "jpf", "jpx", "heic", "heif", "avif", "dng"
    };

    return imageSuffixes.contains(suffix);
}

} // namespace Views
} // namespace GenPreCVSystem
