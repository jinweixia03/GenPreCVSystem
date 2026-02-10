#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QApplication>

#include "modernstyle.h"

namespace GenPreCVSystem {
namespace UI {

/**
 * @brief 主题类型枚举
 */
enum class Theme {
    Dark,      ///< 深色主题
    Light      ///< 浅色主题
};

/**
 * @brief 主题管理器
 *
 * 管理应用程序的主题切换和样式应用
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取主题管理器单例
     */
    static ThemeManager* instance();

    /**
     * @brief 设置当前主题
     * @param theme 主题类型
     */
    void setTheme(Theme theme);

    /**
     * @brief 获取当前主题
     */
    Theme currentTheme() const { return m_currentTheme; }

    /**
     * @brief 切换主题（深色/浅色）
     */
    void toggleTheme();

    /**
     * @brief 应用主题到应用程序
     */
    void applyTheme();

signals:
    /**
     * @brief 主题已改变信号
     */
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);

    Theme m_currentTheme;
};

} // namespace UI
} // namespace GenPreCVSystem

#endif // THEMEMANAGER_H
