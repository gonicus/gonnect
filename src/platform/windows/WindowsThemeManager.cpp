#include "WindowsThemeManager.h"
#include <QLoggingCategory>
#include <QDebug>
#include <QEventLoop>
#include <QtCore/qwineventnotifier.h>
#include <QtCore/private/qsystemerror_p.h>

Q_LOGGING_CATEGORY(lcThemeManager, "gonnect.app.background")

ThemeManager &ThemeManager::instance()
{
    static ThemeManager *_instance = nullptr;
    if (!_instance) {
        _instance = new WindowsThemeManager;
    }
    return *_instance;
}

WindowsThemeManager::WindowsThemeManager() : ThemeManager()
{
    WindowsThemeManager::initColorThemeDetection();
}

void WindowsThemeManager::initColorThemeDetection()
{
    ThemeManager::initColorThemeDetection();
    m_themeSettings = new QSettings(
            "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            QSettings::NativeFormat);

    startThemeWatcher();
    updateColorTheme();
}

void WindowsThemeManager::startThemeWatcher()
{
    auto regKey = LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)";
    if (auto status = RegOpenKeyExW(HKEY_CURRENT_USER, regKey, 0, KEY_READ, &m_key);
        status != ERROR_SUCCESS) {
        m_key = nullptr;
        qCWarning(lcThemeManager) << "Failed to open key" << regKey << "due to"
                                  << QSystemError::windowsString(status);
    }

    m_keyChangedEvent.reset(CreateEvent(nullptr, false, false, nullptr));
    auto *notifier = new QWinEventNotifier(m_keyChangedEvent.get(), this);

    auto registerForNotification = [this] {
        constexpr auto changeFilter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES
                | REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_SECURITY;

        if (auto status = RegNotifyChangeKeyValue(m_key, true, changeFilter,
                                                  m_keyChangedEvent.get(), true);
            status != ERROR_SUCCESS) {
            qCWarning(lcThemeManager) << "Failed to register notification for registry key" << this
                                      << "due to" << QSystemError::windowsString(status);
        }
    };

    QObject::connect(notifier, &QWinEventNotifier::activated, this,
                     [this, registerForNotification] {
                         updateColorTheme();
                         registerForNotification();
                     });

    registerForNotification();
}

void WindowsThemeManager::updateColorTheme()
{
    m_themeSettings->sync();
    auto systemLight = m_themeSettings->value("SystemUsesLightTheme").toBool();
    auto scheme = systemLight ? ThemeManager::ColorScheme::LIGHT : ThemeManager::ColorScheme::DARK;
    if (scheme != m_trayColorScheme) {
        m_trayColorScheme = scheme;
        Q_EMIT trayColorSchemeChanged();
    }
}
