#include "filecontroller.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QDateTime>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QPushButton>
#include <QFileInfo>

#include "../utils/clipboardhelper.h"

namespace GenPreCVSystem {
namespace Controllers {

FileController::FileController(QWidget *parentWidget, QObject *parent)
    : QObject(parent)
    , m_parentWidget(parentWidget)
{
}

QString FileController::openImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        m_parentWidget, "打开图片文件", "", Utils::FileUtils::getImageFileFilter()
    );
    return fileName;
}

QString FileController::openFolder()
{
    QString dirPath = QFileDialog::getExistingDirectory(m_parentWidget, "打开图片文件夹");
    return dirPath;
}

QString FileController::saveImage(const QPixmap &pixmap, const QString &filePath)
{
    if (pixmap.isNull()) {
        emit logMessage("没有可保存的图片");
        return QString();
    }

    QFileInfo fileInfo(filePath);

    if (fileInfo.exists()) {
        // 文件存在，显示选项对话框
        QString previewPath = Utils::FileUtils::generateCopyFilePath(filePath);
        QString previewName = QFileInfo(previewPath).fileName();

        int choice;
        if (!showSaveOptionsDialog(previewName, choice)) {
            return QString(); // 用户取消
        }

        if (choice == 1) {
            // 保存副本
            if (pixmap.save(previewPath)) {
                emit logMessage(QString("已保存副本: %1").arg(previewPath));
                emit fileSaved(previewPath);
                return previewPath;
            } else {
                QMessageBox::warning(m_parentWidget, "错误", "保存副本失败");
                return QString();
            }
        }
        // choice == 0: 覆盖原文件，继续执行下面的保存代码
    }

    // 直接保存到原路径
    if (pixmap.save(filePath)) {
        emit logMessage(QString("已保存: %1").arg(filePath));
        emit fileSaved(filePath);
        return filePath;
    } else {
        QMessageBox::warning(m_parentWidget, "错误", "保存失败");
        return QString();
    }
}

QString FileController::saveImageAs(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        QMessageBox::warning(m_parentWidget, "提示", "没有可保存的图片");
        return QString();
    }

    QString fileName = QFileDialog::getSaveFileName(
        m_parentWidget, "另存为", "", Utils::FileUtils::getImageFileFilter()
    );

    if (!fileName.isEmpty()) {
        if (pixmap.save(fileName)) {
            emit logMessage(QString("已保存: %1").arg(fileName));
            emit fileSaved(fileName);
            return fileName;
        } else {
            QMessageBox::warning(m_parentWidget, "错误", "保存失败");
            return QString();
        }
    }

    return QString();
}

QString FileController::exportImage(const QPixmap &pixmap, const QString &originalPath)
{
    if (pixmap.isNull()) {
        QMessageBox::warning(m_parentWidget, "提示", "没有可导出的图片");
        return QString();
    }

    QString defaultName = originalPath.isEmpty()
        ? QString("exported_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
        : QFileInfo(originalPath).baseName() + "_exported.png";

    QString fileName = QFileDialog::getSaveFileName(
        m_parentWidget, "导出图片", defaultName, Utils::FileUtils::getImageFileFilter()
    );

    if (!fileName.isEmpty()) {
        if (pixmap.save(fileName)) {
            emit logMessage(QString("图片已导出: %1").arg(fileName));
            QMessageBox::information(m_parentWidget, "成功", "图片导出成功！");
            return fileName;
        } else {
            QMessageBox::warning(m_parentWidget, "错误", "图片导出失败！");
            return QString();
        }
    }

    return QString();
}

QString FileController::copyFile(const QString &sourcePath, const QString &targetDir)
{
    QFileInfo sourceInfo(sourcePath);
    QString targetPath = QDir(targetDir).absoluteFilePath(sourceInfo.fileName());

    // 检查文件是否存在
    if (QFileInfo::exists(targetPath)) {
        QString previewPath = Utils::FileUtils::generateCopyFilePath(targetPath);
        QString previewName = QFileInfo(previewPath).fileName();

        int choice;
        if (!showSaveOptionsDialog(previewName, choice)) {
            return QString(); // 用户取消
        }

        if (choice == 1) {
            targetPath = previewPath; // 保存副本
        }
        // choice == 0: 覆盖原文件，使用原路径
    }

    // 复制文件
    if (Utils::FileUtils::copyFile(sourcePath, targetPath)) {
        emit logMessage(QString("已复制: %1").arg(QFileInfo(targetPath).fileName()));
        return targetPath;
    } else {
        QMessageBox::warning(m_parentWidget, "错误", QString("无法复制文件: %1").arg(sourceInfo.fileName()));
        return QString();
    }
}

bool FileController::deleteFile(const QString &filePath)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        m_parentWidget, "确认删除",
        QString("确定要删除文件 \"%1\" 吗？").arg(QFileInfo(filePath).fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );

    if (reply == QMessageBox::No) {
        return false;
    }

    if (Utils::FileUtils::deleteFile(filePath)) {
        emit logMessage(QString("已删除文件: %1").arg(filePath));
        emit fileDeleted(filePath);
        return true;
    } else {
        QMessageBox::critical(m_parentWidget, "删除失败", "无法删除文件。");
        return false;
    }
}

QString FileController::renameFile(const QString &oldPath)
{
    QFileInfo fileInfo(oldPath);

    bool ok;
    QString newName = QInputDialog::getText(
        m_parentWidget, "重命名文件", "新文件名:",
        QLineEdit::Normal, fileInfo.fileName(), &ok
    );

    if (!ok || newName.isEmpty() || newName == fileInfo.fileName()) {
        return QString();
    }

    QString newPath = fileInfo.absolutePath() + "/" + newName;

    if (QFile::exists(newPath)) {
        QMessageBox::warning(m_parentWidget, "错误", "目标文件名已存在。");
        return QString();
    }

    if (Utils::FileUtils::renameFile(oldPath, newPath)) {
        emit logMessage(QString("文件已重命名: %1 -> %2").arg(fileInfo.fileName(), newName));
        return newPath;
    } else {
        QMessageBox::critical(m_parentWidget, "重命名失败", "无法重命名文件。");
        return QString();
    }
}

bool FileController::deleteFolder(const QString &folderPath)
{
    QFileInfo folderInfo(folderPath);

    QMessageBox::StandardButton reply = QMessageBox::question(
        m_parentWidget, "确认删除",
        QString("确定要删除文件夹 \"%1\" 及其所有内容吗？").arg(folderInfo.fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );

    if (reply == QMessageBox::No) {
        return false;
    }

    if (Utils::FileUtils::deleteFolder(folderPath)) {
        emit logMessage(QString("已删除文件夹: %1").arg(folderPath));
        return true;
    } else {
        QMessageBox::critical(m_parentWidget, "删除失败", "无法删除文件夹。");
        return false;
    }
}

QString FileController::createFolder(const QString &parentPath)
{
    bool ok;
    QString folderName = QInputDialog::getText(
        m_parentWidget, "新建文件夹", "文件夹名称:",
        QLineEdit::Normal, "新建文件夹", &ok
    );

    if (!ok || folderName.isEmpty()) {
        return QString();
    }

    QDir currentDir(parentPath);
    QString newFolderPath = currentDir.absoluteFilePath(folderName);

    if (QFileInfo::exists(newFolderPath)) {
        QMessageBox::warning(m_parentWidget, "错误", "同名文件夹已存在。");
        return QString();
    }

    if (Utils::FileUtils::createFolder(parentPath, folderName)) {
        emit logMessage(QString("已创建文件夹: %1").arg(folderName));
        return newFolderPath;
    } else {
        QMessageBox::critical(m_parentWidget, "创建失败", "无法创建文件夹。");
        return QString();
    }
}

QString FileController::pasteImageToDir(const QString &targetDir, QPixmap &pixmap)
{
    QString fileName;
    if (!Utils::ClipboardHelper::pasteImage(pixmap, fileName)) {
        QMessageBox::warning(m_parentWidget, "提示", "剪贴板中没有图片");
        return QString();
    }

    QString targetPath = QDir(targetDir).absoluteFilePath(fileName);

    // 检查文件是否存在
    if (QFileInfo::exists(targetPath)) {
        QString previewPath = Utils::FileUtils::generateCopyFilePath(targetPath);
        QString previewName = QFileInfo(previewPath).fileName();

        int choice;
        if (!showSaveOptionsDialog(previewName, choice)) {
            return QString();
        }

        if (choice == 1) {
            targetPath = previewPath;
        }
    }

    // 保存图片
    if (pixmap.save(targetPath)) {
        emit logMessage(QString("已粘贴图片: %1").arg(QFileInfo(targetPath).fileName()));
        return targetPath;
    } else {
        QMessageBox::critical(m_parentWidget, "错误", "保存图片失败");
        return QString();
    }
}

bool FileController::showSaveOptionsDialog(const QString &previewFileName, int &outChoice)
{
    QDialog dialog(m_parentWidget);
    dialog.setWindowTitle("保存选项");
    dialog.resize(450, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    // 信息提示
    QLabel *infoLabel = new QLabel("文件已存在，请选择保存方式：", &dialog);
    mainLayout->addWidget(infoLabel);

    // 选项组
    QGroupBox *optionsGroup = new QGroupBox("保存选项", &dialog);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);

    QRadioButton *overwriteRadio = new QRadioButton("覆盖原文件", &dialog);
    QRadioButton *copyRadio = new QRadioButton("保存副本", &dialog);
    copyRadio->setChecked(true); // 默认选中保存副本

    optionsLayout->addWidget(overwriteRadio);
    optionsLayout->addWidget(copyRadio);

    // 副本文件名预览
    QWidget *previewWidget = new QWidget(&dialog);
    QHBoxLayout *previewLayout = new QHBoxLayout(previewWidget);
    previewLayout->setContentsMargins(20, 0, 0, 0);

    QLabel *previewLabel = new QLabel("新文件名:", &dialog);
    QLabel *previewValue = new QLabel(previewFileName, &dialog);
    previewValue->setStyleSheet("font-weight: bold; color: #2196F3;");

    previewLayout->addWidget(previewLabel);
    previewLayout->addWidget(previewValue);
    previewLayout->addStretch();

    optionsLayout->addWidget(previewWidget);

    mainLayout->addWidget(optionsGroup);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *saveButton = new QPushButton("保存", &dialog);
    QPushButton *cancelButton = new QPushButton("取消", &dialog);

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        // 返回用户选择
        outChoice = overwriteRadio->isChecked() ? 0 : 1;
        return true;
    }

    return false;
}

} // namespace Controllers
} // namespace GenPreCVSystem
