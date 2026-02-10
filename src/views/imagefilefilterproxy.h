#ifndef IMAGEFILEFILTERPROXY_H
#define IMAGEFILEFILTERPROXY_H

#include <QSortFilterProxyModel>

namespace GenPreCVSystem {
namespace Views {

/**
 * @brief 图片文件过滤代理模型
 *
 * 继承自 QSortFilterProxyModel，用于过滤文件系统模型，
 * 只显示图片格式的文件（如 png, jpg, jpeg, bmp, gif, tiff 等）
 */
class ImageFileFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ImageFileFilterProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

} // namespace Views
} // namespace GenPreCVSystem

#endif // IMAGEFILEFILTERPROXY_H
