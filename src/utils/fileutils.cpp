#include "fileutils.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>

namespace GenPreCVSystem {
namespace Utils {

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

} // namespace Utils
} // namespace GenPreCVSystem
