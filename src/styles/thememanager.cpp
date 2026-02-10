#include "thememanager.h"

namespace GenPreCVSystem {
namespace UI {

ThemeManager* ThemeManager::instance()
{
    static ThemeManager instance;
    return &instance;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , m_currentTheme(Theme::Dark)
{
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_currentTheme != theme) {
        m_currentTheme = theme;
        applyTheme();
        emit themeChanged(theme);
    }
}

void ThemeManager::toggleTheme()
{
    setTheme(m_currentTheme == Theme::Dark ? Theme::Light : Theme::Dark);
}

void ThemeManager::applyTheme()
{
    QString styleSheet;

    switch (m_currentTheme) {
        case Theme::Dark:
            styleSheet = ModernStyle::darkTheme();
            break;
        case Theme::Light:
            styleSheet = ModernStyle::lightTheme();
            break;
    }

    qApp->setStyleSheet(styleSheet);
}

} // namespace UI
} // namespace GenPreCVSystem
