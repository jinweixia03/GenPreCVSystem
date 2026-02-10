#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>
#include <QPixmap>
#include <QDir>

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 文件操作工具类
 *
 * 提供文件保存、复制、删除、重命名等操作
 */
class FileUtils
{
public:
    /**
     * @brief 保存选项
     */
    enum SaveOption {
        Overwrite,    ///< 覆盖原文件
        SaveCopy,     ///< 保存副本
        Cancel        ///< 取消
    };

    /**
     * @brief 生成副本文件路径
     * @param originalPath 原始文件路径
     * @return 副本文件路径
     */
    static QString generateCopyFilePath(const QString &originalPath);

    /**
     * @brief 获取已存在的副本编号列表
     * @param baseName 基础文件名
     * @param dir 文件目录
     * @param suffix 文件扩展名
     * @return 已使用的副本编号列表
     */
    static QList<int> getExistingCopyNumbers(const QString &baseName,
                                             const QDir &dir,
                                             const QString &suffix);

    /**
     * @brief 保存图片到文件
     * @param pixmap 图片数据
     * @param filePath 目标文件路径
     * @return true 成功，false 失败
     */
    static bool saveImage(const QPixmap &pixmap, const QString &filePath);

    /**
     * @brief 复制文件
     * @param sourcePath 源文件路径
     * @param targetPath 目标文件路径
     * @return true 成功，false 失败
     */
    static bool copyFile(const QString &sourcePath, const QString &targetPath);

    /**
     * @brief 删除文件
     * @param filePath 文件路径
     * @return true 成功，false 失败
     */
    static bool deleteFile(const QString &filePath);

    /**
     * @brief 重命名文件
     * @param oldPath 旧文件路径
     * @param newPath 新文件路径
     * @return true 成功，false 失败
     */
    static bool renameFile(const QString &oldPath, const QString &newPath);

    /**
     * @brief 删除文件夹及其内容
     * @param folderPath 文件夹路径
     * @return true 成功，false 失败
     */
    static bool deleteFolder(const QString &folderPath);

    /**
     * @brief 创建文件夹
     * @param parentPath 父目录路径
     * @param folderName 文件夹名称
     * @return true 成功，false 失败
     */
    static bool createFolder(const QString &parentPath, const QString &folderName);

    /**
     * @brief 获取支持的图片格式过滤器字符串
     * @return 图片格式过滤器字符串
     */
    static QString getImageFileFilter();

    /**
     * @brief 检查文件是否为图片
     * @param filePath 文件路径
     * @return true 是图片，false 不是图片
     */
    static bool isImageFile(const QString &filePath);
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // FILEUTILS_H
