#ifndef CLIPBOARDHELPER_H
#define CLIPBOARDHELPER_H

#include <QPixmap>
#include <QString>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 剪贴板工具类
 *
 * 处理图片的复制和粘贴操作
 */
class ClipboardHelper
{
public:
    /**
     * @brief 复制图片到剪贴板
     * @param pixmap 图片数据
     * @param filePath 文件路径（可选，用于文件引用）
     */
    static void copyImage(const QPixmap &pixmap, const QString &filePath = QString());

    /**
     * @brief 从剪贴板获取图片
     * @param outPixmap 输出图片数据
     * @param outFileName 输出文件名（如果有的话）
     * @return true 成功获取，false 剪贴板中没有图片
     */
    static bool pasteImage(QPixmap &outPixmap, QString &outFileName);

    /**
     * @brief 检查剪贴板是否有图片
     * @return true 有图片，false 无图片
     */
    static bool hasImage();

private:
    /**
     * @brief 从URL列表中获取第一个图片文件
     */
    static bool getImageFromUrls(const QList<QUrl> &urls, QPixmap &outPixmap, QString &outFileName);

    /**
     * @brief 从图片数据中获取图片
     */
    static bool getImageFromData(const QMimeData *mimeData, QPixmap &outPixmap, QString &outFileName);
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // CLIPBOARDHELPER_H
