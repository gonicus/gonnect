#pragma once
#include <QObject>
#include <QColor>
#include "PortalSettings.h"
#include "ThemeManager.h"

class SettingsPortal : public QObject
{
    Q_OBJECT

public:
    static SettingsPortal &instance()
    {
        static SettingsPortal *_instance = nullptr;
        if (!_instance) {
            _instance = new SettingsPortal();
        }
        return *_instance;
    }

    ThemeManager::ColorScheme colorScheme() const { return m_colorScheme; }
    QColor accentColor() const { return m_accentColor; }
    bool highContrast() const { return m_highContrast; }

signals:
    void colorSchemeChanged();
    void accentColorChanged();
    void highContrastChanged();

private:
    SettingsPortal(QObject *parent = nullptr);

    void settingsChanged(QString ns, QString key, QDBusVariant value);

    ThemeManager::ColorScheme unsignedToColorScheme(unsigned value);

    OrgFreedesktopPortalSettingsInterface *m_portal;

    ThemeManager::ColorScheme m_colorScheme = ThemeManager::ColorScheme::NO_PREFERENCE;
    QColor m_accentColor;

    bool m_highContrast = false;

    Q_DISABLE_COPY(SettingsPortal)
};
