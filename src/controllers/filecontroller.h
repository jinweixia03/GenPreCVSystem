#ifndef FILECONTROLLER_H
#define FILECONTROLLER_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QWidget>

#include "../utils/fileutils.h"

namespace GenPreCVSystem {
namespace Controllers {

/**
 * @brief 文件操作控制器
 *
 * 管理文件的保存、另存为、复制、删除等操作
 */
class FileController : public QObject
{
    Q_OBJECT

public:
    explicit FileController(QWidget *parentWidget, QObject *parent = nullptr);

    /**
     * @brief 打开图片文件
     * @return 文件路径，如果取消则返回空字符串
     */
    QString openImage();

    /**
     * @brief 打开文件夹
     * @return 文件夹路径，如果取消则返回空字符串
     */
    QString openFolder();

    /**
     * @brief 保存图片
     * @param pixmap 图片数据
     * @param filePath 原始文件路径（用于判断是否覆盖）
     * @return 保存后的文件路径
     */
    QString saveImage(const QPixmap &pixmap, const QString &filePath);

    /**
     * @brief 另存为图片
     * @param pixmap 图片数据
     * @return 保存后的文件路径，如果取消则返回空字符串
     */
    QString saveImageAs(const QPixmap &pixmap);

    /**
     * @brief 导出图片
     * @param pixmap 图片数据
     * @param originalPath 原始文件路径（用于生成默认文件名）
     * @return 导出后的文件路径，如果取消则返回空字符串
     */
    QString exportImage(const QPixmap &pixmap, const QString &originalPath = QString());

    /**
     * @brief 复制文件
     * @param sourcePath 源文件路径
     * @param targetDir 目标目录
     * @return 复制后的文件路径
     */
    QString copyFile(const QString &sourcePath, const QString &targetDir);

    /**
     * @brief 删除文件
     * @param filePath 文件路径
     * @return true 成功，false 失败
     */
    bool deleteFile(const QString &filePath);

    /**
     * @brief 重命名文件
     * @param oldPath 旧文件路径
     * @return 新文件路径，如果取消则返回空字符串
     */
    QString renameFile(const QString &oldPath);

    /**
     * @brief 删除文件夹
     * @param folderPath 文件夹路径
     * @return true 成功，false 失败
     */
    bool deleteFolder(const QString &folderPath);

    /**
     * @brief 创建文件夹
     * @param parentPath 父目录路径
     * @return 新文件夹路径，如果取消则返回空字符串
     */
    QString createFolder(const QString &parentPath);

    /**
     * @brief 粘贴图片到目录
     * @param targetDir 目标目录
     * @param pixmap 输出图片数据
     * @return 保存后的文件路径，如果失败返回空字符串
     */
    QString pasteImageToDir(const QString &targetDir, QPixmap &pixmap);

signals:
    void logMessage(const QString &message);
    void fileSaved(const QString &filePath);
    void fileDeleted(const QString &filePath);

private:
    bool showSaveOptionsDialog(const QString &previewFileName, int &outChoice);

    QWidget *m_parentWidget;
};

} // namespace Controllers
} // namespace GenPreCVSystem

#endif // FILECONTROLLER_H
