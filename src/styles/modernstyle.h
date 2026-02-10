#ifndef MODERNSTYLE_H
#define MODERNSTYLE_H

#include <QString>

namespace GenPreCVSystem {
namespace UI {

/**
 * @brief 现代化样式管理器
 *
 * 参考ANSA 2025风格，提供深色主题的现代化UI样式
 */
class ModernStyle
{
public:
    /**
     * @brief 获取深色主题样式表
     */
    static QString darkTheme();

    /**
     * @brief 获取浅色主题样式表
     */
    static QString lightTheme();

    /**
     * @brief 获取按钮样式
     */
    static QString buttonStyle(bool dark = true);

    /**
     * @brief 获取菜单样式
     */
    static QString menuStyle(bool dark = true);

    /**
     * @brief 获取停靠窗口样式
     */
    static QString dockWidgetStyle(bool dark = true);

    /**
     * @brief 获取标签页样式
     */
    static QString tabWidgetStyle(bool dark = true);

    /**
     * @brief 获取滚动条样式
     */
    static QString scrollBarStyle(bool dark = true);

    /**
     * @brief 获取文本框样式
     */
    static QString textEditStyle(bool dark = true);

    /**
     * @brief 获取树视图样式
     */
    static QString treeViewStyle(bool dark = true);

    /**
     * @brief 获取分组框样式
     */
    static QString groupBoxStyle(bool dark = true);

    /**
     * @brief 获取输入框样式
     */
    static QString lineEditStyle(bool dark = true);

    /**
     * @brief 获取下拉框样式
     */
    static QString comboBoxStyle(bool dark = true);

    /**
     * @brief 获取滑块样式
     */
    static QString sliderStyle(bool dark = true);

    /**
     * @brief 获取复选框样式
     */
    static QString checkBoxStyle(bool dark = true);

    /**
     * @brief 获取进度条样式
     */
    static QString progressBarStyle(bool dark = true);

    /**
     * @brief 获取工具提示样式
     */
    static QString toolTipStyle(bool dark = true);
};

} // namespace UI
} // namespace GenPreCVSystem

#endif // MODERNSTYLE_H
