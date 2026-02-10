#ifndef TABDATA_H
#define TABDATA_H

#include <QPixmap>
#include <QStack>

namespace GenPreCVSystem {
namespace Models {

/**
 * @brief 标签页数据模型
 *
 * 存储每个标签页的图片数据、路径和撤销/重做栈
 */
struct TabData
{
    QString imagePath;        ///< 图片文件路径
    QPixmap pixmap;           ///< 图片数据
    QStack<QPixmap> undoStack;///< 撤销栈
    QStack<QPixmap> redoStack;///< 重做栈

    TabData() = default;

    TabData(const QString &path, const QPixmap &pix)
        : imagePath(path), pixmap(pix)
    {}
};

} // namespace Models
} // namespace GenPreCVSystem

#endif // TABDATA_H
