#ifndef TABCONTROLLER_H
#define TABCONTROLLER_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QHash>
#include <QTabWidget>

#include "../models/tabdata.h"
#include "../models/undostack.h"
#include "../views/imageview.h"

namespace GenPreCVSystem {
namespace Controllers {

/**
 * @brief 标签页控制器
 *
 * 管理图片标签页的创建、切换、关闭等操作
 */
class TabController : public QObject
{
    Q_OBJECT

public:
    explicit TabController(QTabWidget *tabWidget, QObject *parent = nullptr);
    ~TabController();

    /**
     * @brief 加载并显示图片
     * @param filePath 图片文件路径
     * @return true 成功加载，false 加载失败
     */
    bool loadImage(const QString &filePath);

    /**
     * @brief 关闭当前标签页
     */
    void closeCurrentTab();

    /**
     * @brief 关闭指定标签页
     * @param index 标签页索引
     */
    void closeTab(int index);

    /**
     * @brief 切换到指定标签页
     * @param index 标签页索引
     */
    void switchToTab(int index);

    /**
     * @brief 获取当前标签页索引
     */
    int currentTabIndex() const;

    /**
     * @brief 获取标签页数量
     */
    int tabCount() const;

    /**
     * @brief 获取当前标签页的ImageView
     */
    Views::ImageView* currentImageView() const;

    /**
     * @brief 获取当前图片路径
     */
    QString currentImagePath() const;

    /**
     * @brief 获取当前图片
     */
    QPixmap currentPixmap() const;

    /**
     * @brief 更新当前图片
     */
    void updateCurrentPixmap(const QPixmap &pixmap);

    /**
     * @brief 获取当前撤销栈
     */
    Models::UndoStack* currentUndoStack();

    /**
     * @brief 检查图片是否已打开
     * @param filePath 图片文件路径
     * @return 如果已打开返回标签页索引，否则返回-1
     */
    int findTabIndex(const QString &filePath);

    /**
     * @brief 保存状态到当前撤销栈
     */
    void saveState();

signals:
    /**
     * @brief 当前标签页改变信号
     */
    void currentTabChanged(int index);

    /**
     * @brief 标签页关闭请求信号
     */
    void tabCloseRequested(int index);

    /**
     * @brief 图片加载信号
     */
    void imageLoaded(const QString &filePath, const QPixmap &pixmap);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);

private:
    void updateCurrentTabRef();
    QString getTabTitle(int index) const;

    QTabWidget *m_tabWidget;
    QHash<int, Models::TabData> m_tabData;

    // 当前活动标签页引用
    QString m_currentImagePath;
    QPixmap m_currentPixmap;
    Models::UndoStack *m_currentUndoStack;
};

} // namespace Controllers
} // namespace GenPreCVSystem

#endif // TABCONTROLLER_H
