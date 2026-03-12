#include "fileutils.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QObject>
#include <algorithm>

namespace GenPreCVSystem {
namespace Utils {

// ========== 常量定义 ==========

const QStringList FileUtils::SUPPORTED_IMAGE_EXTENSIONS = {
    ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tiff", ".tif",
    ".webp", ".ico", ".svg", ".ppm", ".pgm", ".pbm", ".pnm",
    ".xbm", ".xpm", ".jp2", ".j2k", ".jpf", ".jpx",
    ".heic", ".heif", ".avif", ".dng"
};

const QStringList FileUtils::SUPPORTED_MODEL_EXTENSIONS = {
    ".pt", ".pth", ".onnx", ".engine", ".mlmodel"
};

// ========== 文件操作函数 ==========

QString FileUtils::generateCopyFilePath(const QString &originalPath)
{
    QFileInfo fileInfo(originalPath);
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.suffix();
    QDir dir = fileInfo.absoluteDir();

    // 获取已存在的副本编号
    QList<int> existingNumbers = getExistingCopyNumbers(baseName, dir, suffix);

    // 找到最小可用编号
    int copyNumber = 1;
    for (int num : existingNumbers) {
        if (num == copyNumber) {
            copyNumber++;
        } else {
            break; // 找到空缺编号
        }
    }

    // 构造新文件名
    QString newFileName = QString("%1_副本%2.%3")
        .arg(baseName)
        .arg(copyNumber)
        .arg(suffix);

    return dir.absoluteFilePath(newFileName);
}

QList<int> FileUtils::getExistingCopyNumbers(const QString &baseName,
                                              const QDir &dir,
                                              const QString &suffix)
{
    QList<int> numbers;

    // 构建正则表达式匹配副本文件
    QRegularExpression regex(QString("^%1_副本(\\d+)\\.%2$")
        .arg(QRegularExpression::escape(baseName))
        .arg(QRegularExpression::escape(suffix)));

    // 获取目录中所有文件
    QFileInfoList fileList = dir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot, QDir::NoSort
    );

    // 匹配并提取编号
    for (const QFileInfo &fileInfo : fileList) {
        QRegularExpressionMatch match = regex.match(fileInfo.fileName());
        if (match.hasMatch()) {
            int number = match.captured(1).toInt();
            numbers.append(number);
        }
    }

    // 排序编号列表
    std::sort(numbers.begin(), numbers.end());

    return numbers;
}

bool FileUtils::saveImage(const QPixmap &pixmap, const QString &filePath)
{
    return pixmap.save(filePath);
}

bool FileUtils::copyFile(const QString &sourcePath, const QString &targetPath)
{
    return QFile::copy(sourcePath, targetPath);
}

bool FileUtils::deleteFile(const QString &filePath)
{
    return QFile::remove(filePath);
}

bool FileUtils::renameFile(const QString &oldPath, const QString &newPath)
{
    QFile file(oldPath);
    return file.rename(newPath);
}

bool FileUtils::deleteFolder(const QString &folderPath)
{
    QDir dir(folderPath);
    return dir.removeRecursively();
}

bool FileUtils::createFolder(const QString &parentPath, const QString &folderName)
{
    QDir dir(parentPath);
    return dir.mkdir(folderName);
}

QString FileUtils::getImageFileFilter()
{
    return "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.tif *.webp *.ico *.svg *.ppm *.pgm *.pbm *.pnm *.xbm *.xpm *.jp2 *.j2k *.jpf *.jpx *.heic *.heif *.avif *.dng);;All Files (*)";
}

bool FileUtils::isImageFile(const QString &filePath)
{
    return !QPixmap(filePath).isNull();
}

// ========== 安全验证函数实现 ==========

bool FileUtils::isValidFilePath(const QString &filePath, const QString &basePath)
{
    // 检查空路径
    if (filePath.isEmpty()) {
        return false;
    }

    // 检查路径长度
    if (filePath.length() > MAX_PATH_LENGTH) {
        return false;
    }

    // 检查空字节
    if (filePath.contains('\0')) {
        return false;
    }

    // 规范化路径
    QString normalizedPath = QDir::cleanPath(filePath);

    // 检查路径遍历攻击（包含 ..）
    // 1. 路径规范化后不应以 .. 开头
    if (normalizedPath.startsWith("..")) {
        return false;
    }

    // 2. 检查原始路径中的 .. 是否试图访问父目录
    if (filePath.contains("../") || filePath.contains("..\\")) {
        // 如果有 basePath，检查是否在允许范围内
        if (!basePath.isEmpty()) {
            QString absoluteBase = QFileInfo(basePath).absoluteFilePath();
            QString absolutePath = QFileInfo(normalizedPath).absoluteFilePath();
            if (!absolutePath.startsWith(absoluteBase)) {
                return false;
            }
        } else {
            // 没有 basePath 时，不允许任何路径遍历
            return false;
        }
    }

    return true;
}

bool FileUtils::isValidModelPath(const QString &modelPath, QString &errorMsg)
{
    // 路径安全验证
    if (!isValidFilePath(modelPath)) {
        errorMsg = QObject::tr("模型路径不安全或包含非法字符");
        return false;
    }

    // 检查文件是否存在
    QFileInfo fileInfo(modelPath);
    if (!fileInfo.exists()) {
        errorMsg = QObject::tr("模型文件不存在: %1").arg(modelPath);
        return false;
    }

    if (!fileInfo.isFile()) {
        errorMsg = QObject::tr("路径不是文件: %1").arg(modelPath);
        return false;
    }

    // 检查文件扩展名
    QString suffix = fileInfo.suffix().toLower();
    QString fullSuffix = fileInfo.fileName();
    int dotIndex = fullSuffix.lastIndexOf('.');
    if (dotIndex > 0) {
        fullSuffix = fullSuffix.mid(dotIndex).toLower();
    }

    bool validExtension = false;
    for (const QString &ext : SUPPORTED_MODEL_EXTENSIONS) {
        if (modelPath.toLower().endsWith(ext)) {
            validExtension = true;
            break;
        }
    }

    if (!validExtension) {
        errorMsg = QObject::tr("不支持的模型格式，支持的格式: %1")
            .arg(SUPPORTED_MODEL_EXTENSIONS.join(", "));
        return false;
    }

    // 检查文件大小
    qint64 fileSize = fileInfo.size();
    if (fileSize > MAX_MODEL_SIZE) {
        errorMsg = QObject::tr("模型文件过大 (%1 GB > 5 GB)")
            .arg(fileSize / (1024.0 * 1024 * 1024), 0, 'f', 2);
        return false;
    }

    if (fileSize == 0) {
        errorMsg = QObject::tr("模型文件为空");
        return false;
    }

    return true;
}

bool FileUtils::isValidImagePath(const QString &imagePath, QString &errorMsg)
{
    // 路径安全验证
    if (!isValidFilePath(imagePath)) {
        errorMsg = QObject::tr("图像路径不安全或包含非法字符");
        return false;
    }

    // 检查文件是否存在
    QFileInfo fileInfo(imagePath);
    if (!fileInfo.exists()) {
        errorMsg = QObject::tr("图像文件不存在: %1").arg(imagePath);
        return false;
    }

    if (!fileInfo.isFile()) {
        errorMsg = QObject::tr("路径不是文件: %1").arg(imagePath);
        return false;
    }

    // 检查文件扩展名
    QString suffix = fileInfo.suffix().toLower();
    bool validExtension = false;
    for (const QString &ext : SUPPORTED_IMAGE_EXTENSIONS) {
        if (suffix == ext.mid(1)) {  // 移除开头的 .
            validExtension = true;
            break;
        }
    }

    if (!validExtension) {
        errorMsg = QObject::tr("不支持的图像格式: .%1").arg(suffix);
        return false;
    }

    // 检查文件大小
    qint64 fileSize = fileInfo.size();
    if (fileSize > MAX_IMAGE_SIZE) {
        errorMsg = QObject::tr("图像文件过大 (%1 GB > 1 GB)")
            .arg(fileSize / (1024.0 * 1024 * 1024), 0, 'f', 2);
        return false;
    }

    if (fileSize == 0) {
        errorMsg = QObject::tr("图像文件为空");
        return false;
    }

    return true;
}

QString FileUtils::sanitizeFilePath(const QString &filePath)
{
    // 空路径检查
    if (filePath.isEmpty()) {
        return QString();
    }

    // 空字节检查
    if (filePath.contains('\0')) {
        return QString();
    }

    // 规范化路径
    QString normalizedPath = QDir::cleanPath(filePath);

    // 路径安全检查
    if (normalizedPath.startsWith("..")) {
        return QString();
    }

    // 检查路径遍历
    if (filePath.contains("../") || filePath.contains("..\\")) {
        return QString();
    }

    // 检查特殊字符
    static const QRegularExpression invalidChars("[<>:\"|?*\x00-\x1f]");
    if (invalidChars.match(normalizedPath).hasMatch()) {
        return QString();
    }

    return normalizedPath;
}

} // namespace Utils
} // namespace GenPreCVSystem
