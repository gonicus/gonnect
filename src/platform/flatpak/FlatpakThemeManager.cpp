#include "FlatpakThemeManager.h"
#include "SettingsPortal.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcThemeManager, "gonnect.app.background")

ThemeManager &ThemeManager::instance()
{
    static ThemeManager *_instance = nullptr;
    if (!_instance) {
        _instance = new FlatpakThemeManager;
    }
    return *_instance;
}

FlatpakThemeManager::FlatpakThemeManager() : ThemeManager()
{
    FlatpakThemeManager::initColorThemeDetection();
}

FlatpakThemeManager::initColorThemeDetection()
{
    m_settingsPortal = &SettingsPortal::instance();

    connect(m_settingsPortal, &SettingsPortal::colorSchemeChanged, this,
            &FlatpakThemeManager::colorSchemeChanged);
    connect(m_settingsPortal, &SettingsPortal::accentColorChanged, this,
            &FlatpakThemeManager::accentColorChanged);
    connect(m_settingsPortal, &SettingsPortal::highContrastChanged, this,
            &FlatpakThemeManager::highContrastChanged);
}
