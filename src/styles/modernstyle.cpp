#include "modernstyle.h"

namespace GenPreCVSystem {
namespace UI {

QString ModernStyle::darkTheme()
{
    return R"(
        /* 全局样式 */
        QMainWindow {
            background-color: )" DARK_BG R"(;;
        }

        QWidget {
            background-color: )" DARK_BG R"(;
            color: )" DARK_TEXT R"(;
            border: none;
            font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif;
            font-size: 9pt;
        }

        /* 菜单栏 */
        QMenuBar {
            background-color: )" DARK_BG_TERTIARY R"(;
            border-bottom: 1px solid )" DARK_BORDER R"(;
            padding: 2px;
        }

        QMenuBar::item {
            background-color: transparent;
            padding: 6px 12px;
            border-radius: 4px;
        }

        QMenuBar::item:selected {
            background-color: )" DARK_BG_HOVER R"(;
        }

        QMenuBar::item:pressed {
            background-color: )" DARK_BG_PRESSED R"(;
        }

        /* 菜单 */
        QMenu {
            background-color: )" DARK_BG_TERTIARY R"(;
            border: 1px solid )" DARK_BORDER R"(;
            border-radius: 6px;
            padding: 4px;
        }

        QMenu::item {
            padding: 8px 32px 8px 16px;
            border-radius: 4px;
        }

        QMenu::item:selected {
            background-color: )" DARK_BG_HOVER R"(;
        }

        QMenu::separator {
            height: 1px;
            background: )" DARK_BORDER R"(;
            margin: 6px 8px;
        }

        /* 工具栏 */
        QToolBar {
            background-color: )" DARK_BG_TERTIARY R"(;
            border: none;
            spacing: 4px;
            padding: 4px;
        }

        QToolBar::separator {
            width: 1px;
            background-color: )" DARK_BORDER R"(;
            margin: 4px 8px;
        }

        /* 状态栏 */
        QStatusBar {
            background-color: )" DARK_BG_TERTIARY R"(;
            border-top: 1px solid )" DARK_BORDER R"(;
        }

    )" scrollBarStyle(true) +
    dockWidgetStyle(true) +
    tabWidgetStyle(true) +
    buttonStyle(true) +
    textEditStyle(true) +
    treeViewStyle(true) +
    groupBoxStyle(true) +
    lineEditStyle(true) +
    comboBoxStyle(true) +
    sliderStyle(true) +
    checkBoxStyle(true) +
    toolTipStyle(true);
}

QString ModernStyle::lightTheme()
{
    return R"(
        /* 全局样式 */
        QMainWindow {
            background-color: )" LIGHT_BG R"(;
        }

        QWidget {
            background-color: )" LIGHT_BG R"(;
            color: )" LIGHT_TEXT R"(;
            border: none;
            font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif;
            font-size: 9pt;
        }

        /* 菜单栏 */
        QMenuBar {
            background-color: )" LIGHT_BG_SECONDARY R"(;
            border-bottom: 1px solid )" LIGHT_BORDER R"(;
            padding: 2px;
        }

        QMenuBar::item {
            background-color: transparent;
            padding: 6px 12px;
            border-radius: 4px;
        }

        QMenuBar::item:selected {
            background-color: )" LIGHT_BG_HOVER R"(;
        }

        /* 菜单 */
        QMenu {
            background-color: )" LIGHT_BG R"(;
            border: 1px solid )" LIGHT_BORDER R"(;
            border-radius: 6px;
            padding: 4px;
        }

        QMenu::item {
            padding: 8px 32px 8px 16px;
            border-radius: 4px;
        }

        QMenu::item:selected {
            background-color: )" LIGHT_BG_HOVER R"(;
        }

    )" scrollBarStyle(false) +
    dockWidgetStyle(false) +
    tabWidgetStyle(false) +
    buttonStyle(false) +
    textEditStyle(false) +
    treeViewStyle(false) +
    groupBoxStyle(false) +
    lineEditStyle(false) +
    comboBoxStyle(false) +
    sliderStyle(false) +
    checkBoxStyle(false) +
    toolTipStyle(false);
}

QString ModernStyle::buttonStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_TERTIARY : LIGHT_BG_SECONDARY;
    const char* bgHover = dark ? DARK_BG_HOVER : LIGHT_BG_HOVER;
    const char* bgPressed = dark ? DARK_BG_PRESSED : LIGHT_BG_PRESSED;
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;
    const char* accent = dark ? DARK_ACCENT : "#0066CC";

    return QString(R"(
        /* 按钮样式 */
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 6px 16px;
            font-weight: 500;
        }

        QPushButton:hover {
            background-color: %4;
            border: 1px solid %5;
        }

        QPushButton:pressed {
            background-color: %6;
        }

        QPushButton:disabled {
            background-color: %1;
            color: #666666;
            border: 1px solid #444444;
        }

        /* 主要按钮 */
        QPushButton[class="Primary"] {
            background-color: %5;
            color: #FFFFFF;
            border: none;
        }

        QPushButton[class="Primary"]:hover {
            background-color: #1E90FF;
        }

        QPushButton[class="Primary"]:pressed {
            background-color: #0066CC;
        }

        /* 危险按钮 */
        QPushButton[class="Danger"] {
            background-color: #D13438;
            color: #FFFFFF;
            border: none;
        }

        QPushButton[class="Danger"]:hover {
            background-color: #E81123;
        }

        /* 工具按钮 */
        QToolButton {
            background-color: transparent;
            border: none;
            border-radius: 4px;
            padding: 6px;
        }

        QToolButton:hover {
            background-color: %4;
        }

        QToolButton:pressed {
            background-color: %6;
        }
    )").arg(bg, text, border, bgHover, accent, bgPressed);
}

QString ModernStyle::menuStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_TERTIARY : LIGHT_BG;
    const char* bgHover = dark ? DARK_BG_HOVER : LIGHT_BG_HOVER;
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;

    return QString(R"(
        QMenu {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 4px;
        }

        QMenu::item {
            padding: 8px 32px 8px 16px;
            border-radius: 4px;
        }

        QMenu::item:selected {
            background-color: %3;
        }

        QMenu::separator {
            height: 1px;
            background: %2;
            margin: 6px 8px;
        }

        QMenu::indicator {
            width: 16px;
            height: 16px;
            left: 8px;
        }
    )").arg(bg, border, bgHover);
}

QString ModernStyle::dockWidgetStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_SECONDARY : LIGHT_BG_SECONDARY;
    const char* titleBg = dark ? DARK_BG_TERTIARY : LIGHT_BG_TERTIARY;
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;

    return QString(R"(
        QDockWidget {
            background-color: %1;
            color: %4;
            border: 1px solid %3;
            titlebar-close-icon: url(:/icons/close.png);
            titlebar-normal-icon: url(:/icons/undock.png);
            titlebar-float-icon: url(:/icons/dock.png);
        }

        QDockWidget::title {
            background-color: %2;
            padding: 8px;
            border-bottom: 1px solid %3;
        }

        QDockWidget::close-button, QDockWidget::float-button {
            background-color: transparent;
            border: none;
            padding: 4px;
            border-radius: 3px;
        }

        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
            background-color: rgba(255, 255, 255, 0.1);
        }
    )").arg(bg, titleBg, border, text);
}

QString ModernStyle::tabWidgetStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_SECONDARY : LIGHT_BG_SECONDARY;
    const char* tabBg = dark ? DARK_BG : LIGHT_BG;
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;
    const char* accent = dark ? DARK_ACCENT : "#0066CC";

    return QString(R"(
        QTabWidget::pane {
            background-color: %1;
            border: 1px solid %3;
            border-radius: 6px;
        }

        QTabBar::tab {
            background-color: %2;
            color: %4;
            border: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            padding: 8px 16px;
            margin-right: 2px;
        }

        QTabBar::tab:selected {
            background-color: %1;
            color: #FFFFFF;
            border-bottom: 2px solid %5;
        }

        QTabBar::tab:hover:!selected {
            background-color: %6;
        }

        QTabBar::close-button {
            background-color: transparent;
            border: none;
            padding: 2px;
            border-radius: 3px;
        }

        QTabBar::close-button:hover {
            background-color: #D13438;
            color: #FFFFFF;
        }
    )").arg(bg, tabBg, border, dark ? DARK_TEXT : LIGHT_TEXT, accent, dark ? DARK_BG_HOVER : LIGHT_BG_HOVER);
}

QString ModernStyle::scrollBarStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_TERTIARY : "#E0E0E0";
    const char* handle = dark ? "#555555" : "#A0A0A0";
    const char* handleHover = dark ? "#666666" : "#808080";

    return QString(R"(
        QScrollBar:vertical {
            background-color: %1;
            width: 12px;
            border-radius: 6px;
            margin: 0;
        }

        QScrollBar::handle:vertical {
            background-color: %2;
            min-height: 30px;
            border-radius: 6px;
            margin: 2px;
        }

        QScrollBar::handle:vertical:hover {
            background-color: %3;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }

        QScrollBar:horizontal {
            background-color: %1;
            height: 12px;
            border-radius: 6px;
            margin: 0;
        }

        QScrollBar::handle:horizontal {
            background-color: %2;
            min-width: 30px;
            border-radius: 6px;
            margin: 2px;
        }

        QScrollBar::handle:horizontal:hover {
            background-color: %3;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
    )").arg(bg, handle, handleHover);
}

QString ModernStyle::textEditStyle(bool dark)
{
    const char* bg = dark ? "#1A1A1A" : "#FFFFFF";
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;

    return QString(R"(
        QTextEdit {
            background-color: %1;
            color: %3;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Consolas', 'Monaco', monospace;
        }

        QTextEdit:focus {
            border: 1px solid %4;
        }
    )").arg(bg, border, text, dark ? DARK_ACCENT : "#0066CC");
}

QString ModernStyle::treeViewStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_SECONDARY : "#FAFAFA";
    const char* bgHover = dark ? DARK_BG_HOVER : "#E5F3FF";
    const char* bgSelected = dark ? DARK_BG_PRESSED : "#0078D4";
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;

    return QString(R"(
        QTreeView {
            background-color: %1;
            color: %4;
            border: none;
            outline: none;
        }

        QTreeView::item {
            padding: 6px;
            border-radius: 3px;
        }

        QTreeView::item:hover {
            background-color: %2;
        }

        QTreeView::item:selected {
            background-color: %3;
            color: #FFFFFF;
        }

        QTreeView::branch {
            background-color: transparent;
        }

        QTreeView::branch:has-children:!has-siblings:closed,
        QTreeView::branch:closed:has-children:has-siblings {
            border-image: none;
            image: url(:/icons/branch-closed.png);
        }

        QTreeView::branch:open:has-children:!has-siblings,
        QTreeView::branch:open:has-children:has-siblings {
            border-image: none;
            image: url(:/icons/branch-open.png);
        }

        QTreeView::branch:has-children:!has-siblings:closed:hover,
        QTreeView::branch:closed:has-children:has-siblings:hover {
            image: url(:/icons/branch-closed-hover.png);
        }

        QTreeView::branch:open:has-children:!has-siblings:hover,
        QTreeView::branch:open:has-children:has-siblings:hover {
            image: url(:/icons/branch-open-hover.png);
        }

        QTreeView::indicator {
            width: 18px;
            height: 18px;
            border-radius: 3px;
            border: 1px solid #555;
        }

        QTreeView::indicator:checked {
            background-color: %3;
            border: none;
            image: url(:/icons/check.png);
        }
    )").arg(bg, bgHover, bgSelected, text);
}

QString ModernStyle::groupBoxStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_TERTIARY : LIGHT_BG_SECONDARY;
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;

    return QString(R"(
        QGroupBox {
            background-color: %1;
            color: %3;
            border: 1px solid %2;
            border-radius: 6px;
            margin-top: 12px;
            padding: 12px;
            font-weight: 600;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 4px 8px;
        }
    )").arg(bg, border, text);
}

QString ModernStyle::lineEditStyle(bool dark)
{
    const char* bg = dark ? "#1A1A1A" : "#FFFFFF";
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;
    const char* focus = dark ? DARK_ACCENT : "#0066CC";

    return QString(R"(
        QLineEdit {
            background-color: %1;
            color: %3;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 6px 12px;
        }

        QLineEdit:focus {
            border: 1px solid %4;
        }

        QLineEdit:disabled {
            background-color: #2A2A2A;
            color: #666666;
        }
    )").arg(bg, border, text, focus);
}

QString ModernStyle::comboBoxStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_TERTIARY : LIGHT_BG_SECONDARY;
    const char* bgDropdown = dark ? DARK_BG : LIGHT_BG;
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;

    return QString(R"(
        QComboBox {
            background-color: %1;
            color: %4;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 6px 12px;
            min-width: 80px;
        }

        QComboBox:hover {
            border: 1px solid %5;
        }

        QComboBox::drop-down {
            border: none;
            width: 20px;
        }

        QComboBox::down-arrow {
            image: url(:/icons/arrow-down.png);
        }

        QComboBox QAbstractItemView {
            background-color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            selection-background-color: %6;
            selection-color: #FFFFFF;
            padding: 4px;
        }
    )").arg(bg, bgDropdown, border, text, dark ? DARK_BG_HOVER : LIGHT_BG_HOVER, dark ? DARK_BG_PRESSED : "#0078D4");
}

QString ModernStyle::sliderStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_TERTIARY : "#E0E0E0";
    const char* handle = dark ? DARK_ACCENT : "#0066CC";

    return QString(R"(
        QSlider::groove:horizontal {
            height: 6px;
            background-color: %1;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background-color: %2;
            width: 16px;
            height: 16px;
            border-radius: 8px;
            margin: -5px 0;
        }

        QSlider::handle:horizontal:hover {
            background-color: #1E90FF;
        }

        QSlider::groove:vertical {
            width: 6px;
            background-color: %1;
            border-radius: 3px;
        }

        QSlider::handle:vertical {
            background-color: %2;
            width: 16px;
            height: 16px;
            border-radius: 8px;
            margin: 0 -5px;
        }

        QSlider::handle:vertical:hover {
            background-color: #1E90FF;
        }
    )").arg(bg, handle);
}

QString ModernStyle::checkBoxStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_TERTIARY : LIGHT_BG_SECONDARY;
    const char* border = dark ? DARK_BORDER : LIGHT_BORDER;
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;
    const char* accent = dark ? DARK_ACCENT : "#0066CC";

    return QString(R"(
        QCheckBox {
            background-color: transparent;
            color: %3;
            spacing: 8px;
        }

        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            background-color: %1;
            border: 2px solid %2;
            border-radius: 3px;
        }

        QCheckBox::indicator:checked {
            background-color: %4;
            border: none;
            image: url(:/icons/check.png);
        }

        QCheckBox::indicator:hover {
            border: 2px solid %5;
        }
    )").arg(bg, border, text, accent, dark ? DARK_BG_HOVER : "#808080");
}

QString ModernStyle::progressBarStyle(bool dark)
{
    const char* bg = dark ? DARK_BG_TERTIARY : "#E0E0E0";
    const char* value = dark ? DARK_ACCENT : "#0066CC";
    const char* text = dark ? DARK_TEXT : LIGHT_TEXT;

    return QString(R"(
        QProgressBar {
            background-color: %1;
            color: %3;
            border: none;
            border-radius: 8px;
            text-align: center;
            height: 20px;
        }

        QProgressBar::chunk {
            background-color: %2;
            border-radius: 8px;
        }
    )").arg(bg, value, text);
}

QString ModernStyle::toolTipStyle(bool dark)
{
    const char* bg = dark ? "#2D2D30" : "#FFFFE0";
    const char* border = dark ? DARK_BORDER : "#D4D4D4";
    const char* text = dark ? DARK_TEXT : "#1E1E1E";

    return QString(R"(
        QToolTip {
            background-color: %1;
            color: %3;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 6px;
            font-size: 9pt;
        }
    )").arg(bg, border, text);
}

} // namespace UI
} // namespace GenPreCVSystem
