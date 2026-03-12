#ifndef FILETREEVIEW_H
#define FILETREEVIEW_H

#include <QTreeView>
#include <QString>

namespace GenPreCVSystem {
namespace Views {

/**
 * @brief 文件树视图，支持拖放图片
 *
 * 继承自 QTreeView，处理外部文件拖放
 */
class FileTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit FileTreeView(QWidget *parent = nullptr);

    void setTargetPath(const QString &path) { m_targetPath = path; }
    QString targetPath() const { return m_targetPath; }

signals:
    /**
     * @brief 图片文件被拖放到视图
     * @param filePaths 图片文件路径列表
     */
    void imagesDropped(const QStringList &filePaths);

    /**
     * @brief 图片数据被拖放到视图
     * @param imageData 图片数据
     * @param sourceName 源名称（如果有）
     */
    void imageDataDropped(const QByteArray &imageData, const QString &sourceName);

    /**
     * @brief 文件夹被拖放到视图
     * @param folderPath 文件夹路径
     */
    void folderDropped(const QString &folderPath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QString m_targetPath;
};

} // namespace Views
} // namespace GenPreCVSystem

#endif // FILETREEVIEW_H
