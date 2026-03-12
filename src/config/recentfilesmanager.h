#ifndef RECENTFILESMANAGER_H
#define RECENTFILESMANAGER_H

#include <QObject>
#include <QStringList>
#include <QMenu>
#include <QAction>
#include <QList>

namespace GenPreCVSystem {
namespace Utils {

/**
 * @brief 最近文件管理器
 *
 * 管理最近打开的文件列表，自动更新菜单
 */
class RecentFilesManager : public QObject
{
    Q_OBJECT

public:
    static const int DEFAULT_MAX_FILES = 10;

    explicit RecentFilesManager(QObject *parent = nullptr);
    ~RecentFilesManager();

    /**
     * @brief 添加文件到最近列表
     * @param filePath 文件路径
     */
    void addFile(const QString &filePath);

    /**
     * @brief 获取最近文件列表
     */
    QStringList recentFiles() const;

    /**
     * @brief 清除所有最近文件
     */
    void clearRecentFiles();

    /**
     * @brief 设置要更新的菜单
     * @param menu 目标菜单
     * @param beforeAction 在此动作之前插入（可选）
     */
    void setupMenu(QMenu *menu, QAction *beforeAction = nullptr);

    /**
     * @brief 更新菜单动作
     */
    void updateMenuActions();

    /**
     * @brief 设置最大文件数量
     */
    void setMaxFiles(int max);

    /**
     * @brief 获取最大文件数量
     */
    int maxFiles() const { return m_maxFiles; }

signals:
    /**
     * @brief 最近文件被点击信号
     * @param filePath 文件路径
     */
    void recentFileTriggered(const QString &filePath);

    /**
     * @brief 最近文件列表变化信号
     */
    void recentFilesChanged();

private slots:
    void onRecentFileActionTriggered();
    void onClearRecentFilesTriggered();

private:
    void loadFromSettings();
    void saveToSettings();
    void validateFiles();
    QString strippedName(const QString &fullPath) const;

    QStringList m_recentFiles;
    QList<QAction *> m_recentFileActions;
    QAction *m_clearAction;
    QAction *m_separatorAction;
    QMenu *m_menu;
    QAction *m_beforeAction;
    int m_maxFiles;
};

} // namespace Utils
} // namespace GenPreCVSystem

#endif // RECENTFILESMANAGER_H
