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

    // ========== 安全验证函数 ==========

    /**
     * @brief 验证文件路径是否安全（防止路径遍历攻击）
     * @param filePath 文件路径
     * @param basePath 允许的基础路径（可选）
     * @return true 路径安全，false 路径可疑
     *
     * 检查项：
     * - 路径长度限制（最大 4096 字符）
     * - 禁止包含 .. 路径遍历
     * - 禁止空路径
     * - 禁止包含空字节
     */
    static bool isValidFilePath(const QString &filePath, const QString &basePath = QString());

    /**
     * @brief 验证模型文件路径
     * @param modelPath 模型文件路径
     * @param errorMsg 错误信息输出
     * @return true 有效，false 无效
     *
     * 检查项：
     * - 路径安全性
     * - 文件是否存在
     * - 文件扩展名（.pt, .pth, .onnx, .engine, .mlmodel）
     * - 文件大小（最大 5GB）
     */
    static bool isValidModelPath(const QString &modelPath, QString &errorMsg);

    /**
     * @brief 验证图像文件路径
     * @param imagePath 图像文件路径
     * @param errorMsg 错误信息输出
     * @return true 有效，false 无效
     *
     * 检查项：
     * - 路径安全性
     * - 文件是否存在
     * - 文件扩展名
     * - 文件大小（最大 1GB）
     */
    static bool isValidImagePath(const QString &imagePath, QString &errorMsg);

    /**
     * @brief 清理文件路径（规范化并检查安全性）
     * @param filePath 原始文件路径
     * @return 清理后的路径，如果不安全则返回空字符串
     */
    static QString sanitizeFilePath(const QString &filePath);

    /**
     * @brief 支持的图像文件扩展名列表
     */
    static const QStringList SUPPORTED_IMAGE_EXTENSIONS;

    /**
     * @brief 支持的模型文件扩展名列表
     */
    static const QStringList SUPPORTED_MODEL_EXTENSIONS;

    // ========== 常量定义 ==========

    static constexpr int MAX_PATH_LENGTH = 4096;           ///< 最大路径长度
    static constexpr qint64 MAX_IMAGE_SIZE = 1024LL * 1024 * 1024;  ///< 最大图像文件大小 (1GB)
    static constexpr qint64 MAX_MODEL_SIZE = 5LL * 1024 * 1024 * 1024;  ///< 最大模型文件大小 (5GB)
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // FILEUTILS_H
