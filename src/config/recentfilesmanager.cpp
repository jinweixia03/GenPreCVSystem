/**
 * @file recentfilesmanager.cpp
 * @brief 最近文件管理器实现
 *
 * 管理最近打开的文件列表：
 * - 自动保存到设置文件
 * - 更新文件菜单的最近文件子菜单
 * - 验证文件是否仍然存在
 */

#include "recentfilesmanager.h"
#include "appsettings.h"
#include <QFile>
#include <QFileInfo>
#include <QKeySequence>

namespace GenPreCVSystem {
namespace Utils {

RecentFilesManager::RecentFilesManager(QObject *parent)
    : QObject(parent)
    , m_clearAction(nullptr)
    , m_separatorAction(nullptr)
    , m_menu(nullptr)
    , m_beforeAction(nullptr)
    , m_maxFiles(DEFAULT_MAX_FILES)
{
    loadFromSettings();
}

RecentFilesManager::~RecentFilesManager()
{
}

void RecentFilesManager::addFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    // 移除已存在的条目
    m_recentFiles.removeAll(filePath);

    // 添加到列表开头
    m_recentFiles.prepend(filePath);

    // 限制最大数量
    m_maxFiles = AppSettings::maxRecentFiles();
    while (m_recentFiles.size() > m_maxFiles) {
        m_recentFiles.removeLast();
    }

    saveToSettings();
    updateMenuActions();
    emit recentFilesChanged();
}

QStringList RecentFilesManager::recentFiles() const
{
    return m_recentFiles;
}

void RecentFilesManager::clearRecentFiles()
{
    m_recentFiles.clear();
    saveToSettings();
    updateMenuActions();
    emit recentFilesChanged();
}

void RecentFilesManager::setupMenu(QMenu *menu, QAction *beforeAction)
{
    m_menu = menu;
    m_beforeAction = beforeAction;

    // 创建分隔符
    m_separatorAction = new QAction(this);
    m_separatorAction->setSeparator(true);

    // 创建清除动作
    m_clearAction = new QAction(tr("清除最近文件"), this);
    connect(m_clearAction, &QAction::triggered, this, &RecentFilesManager::onClearRecentFilesTriggered);

    // 初始更新菜单
    updateMenuActions();
}

void RecentFilesManager::updateMenuActions()
{
    if (!m_menu) {
        return;
    }

    // 移除旧的最近文件动作
    for (QAction *action : m_recentFileActions) {
        m_menu->removeAction(action);
        delete action;
    }
    m_recentFileActions.clear();

    // 移除分隔符和清除动作
    if (m_separatorAction) {
        m_menu->removeAction(m_separatorAction);
    }
    if (m_clearAction) {
        m_menu->removeAction(m_clearAction);
    }

    // 验证文件存在性
    validateFiles();

    if (m_recentFiles.isEmpty()) {
        return;
    }

    // 添加最近文件动作
    int index = 1;
    for (const QString &file : m_recentFiles) {
        QString text = tr("&%1 %2").arg(index).arg(strippedName(file));
        QAction *action = new QAction(text, this);
        action->setData(file);
        action->setToolTip(file);

        // 添加快捷键 (Ctrl+1, Ctrl+2, ...)
        if (index <= 9) {
            action->setShortcut(QKeySequence(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_0 + index)));
        }

        connect(action, &QAction::triggered, this, &RecentFilesManager::onRecentFileActionTriggered);
        m_recentFileActions.append(action);

        if (m_beforeAction) {
            m_menu->insertAction(m_beforeAction, action);
        } else {
            m_menu->addAction(action);
        }

        index++;
    }

    // 添加分隔符
    if (m_beforeAction) {
        m_menu->insertAction(m_beforeAction, m_separatorAction);
        m_menu->insertAction(m_beforeAction, m_clearAction);
    } else {
        m_menu->addAction(m_separatorAction);
        m_menu->addAction(m_clearAction);
    }
}

void RecentFilesManager::setMaxFiles(int max)
{
    m_maxFiles = max;
    AppSettings::setMaxRecentFiles(max);

    while (m_recentFiles.size() > m_maxFiles) {
        m_recentFiles.removeLast();
    }

    saveToSettings();
    updateMenuActions();
}

void RecentFilesManager::onRecentFileActionTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        QString filePath = action->data().toString();
        if (!filePath.isEmpty()) {
            emit recentFileTriggered(filePath);
        }
    }
}

void RecentFilesManager::onClearRecentFilesTriggered()
{
    clearRecentFiles();
}

void RecentFilesManager::loadFromSettings()
{
    m_recentFiles = AppSettings::recentFiles();
    m_maxFiles = AppSettings::maxRecentFiles();
    validateFiles();
}

void RecentFilesManager::saveToSettings()
{
    AppSettings::setRecentFiles(m_recentFiles);
}

void RecentFilesManager::validateFiles()
{
    QStringList validFiles;
    for (const QString &file : m_recentFiles) {
        if (QFile::exists(file)) {
            validFiles.append(file);
        }
    }
    m_recentFiles = validFiles;
}

QString RecentFilesManager::strippedName(const QString &fullPath) const
{
    return QFileInfo(fullPath).fileName();
}

} // namespace Utils
} // namespace GenPreCVSystem
