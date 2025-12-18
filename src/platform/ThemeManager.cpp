#include "ThemeManager.h"

#include <QStyleHints>
#include <QGuiApplication>

ThemeManager::ThemeManager() : QObject()
{
    ThemeManager::initColorThemeDetection();
}

void ThemeManager::initColorThemeDetection()
{
    auto styleHints = QGuiApplication::styleHints();
    connect(styleHints, &QStyleHints::colorSchemeChanged, this,
            &ThemeManager::handleColorSchemeChange);
    handleColorSchemeChange(styleHints->colorScheme());

#if defined(Q_OS_LINUX)
    const auto desktop = QString::fromLocal8Bit(qgetenv("XDG_SESSION_DESKTOP")).toLower();
    if (desktop.contains("kde")) {
        m_trayColorScheme = ThemeManager::ColorScheme::LIGHT;
    } else if (desktop.contains("gnome")) {
        m_trayColorScheme = ThemeManager::ColorScheme::DARK;
    }
#endif
}

void ThemeManager::handleColorSchemeChange(Qt::ColorScheme colorScheme)
{
    auto scheme = ThemeManager::ColorScheme::NO_PREFERENCE;
    switch (colorScheme) {
    case Qt::ColorScheme::Dark:
        scheme = ThemeManager::ColorScheme::DARK;
        break;

    case Qt::ColorScheme::Light:
        scheme = ThemeManager::ColorScheme::LIGHT;
        break;

    default:
        scheme = ThemeManager::ColorScheme::NO_PREFERENCE;
        break;
    }
    if (scheme != m_colorScheme) {
        m_colorScheme = scheme;
        Q_EMIT colorSchemeChanged();
    }
}

ThemeManager::ColorScheme ThemeManager::trayColorScheme() const
{
    if (m_trayColorScheme != ThemeManager::ColorScheme::NO_PREFERENCE) {
        return m_trayColorScheme;
    }
    return m_colorScheme;
}
