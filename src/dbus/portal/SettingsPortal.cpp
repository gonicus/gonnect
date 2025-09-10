#include <QDBusConnection>
#include "SettingsPortal.h"

SettingsPortal::SettingsPortal(QObject *parent) : QObject(parent)
{
    m_portal = new OrgFreedesktopPortalSettingsInterface("org.freedesktop.portal.Desktop",
                                                         "/org/freedesktop/portal/desktop",
                                                         QDBusConnection::sessionBus(), this);

    QTimer::singleShot(0, this, [this]() {
        auto reply = m_portal->ReadOne("org.freedesktop.appearance", "color-scheme");
        reply.waitForFinished();
        if (reply.isValid()) {
            unsigned cs = reply.value().variant().toUInt();
            m_colorScheme = unsignedToColorScheme(cs);
            Q_EMIT colorSchemeChanged();
        }

        reply = m_portal->ReadOne("org.freedesktop.appearance", "contrast");
        reply.waitForFinished();
        if (reply.isValid()) {
            m_highContrast = reply.value().variant().toBool();
            Q_EMIT highContrastChanged();
        }
    });

    connect(m_portal, &OrgFreedesktopPortalSettingsInterface::SettingChanged, this,
            &SettingsPortal::settingsChanged);
}

ThemeManager::ColorScheme SettingsPortal::unsignedToColorScheme(unsigned value)
{
    if (value == 1) {
        return ThemeManager::ColorScheme::DARK;
    }
    if (value == 2) {
        return ThemeManager::ColorScheme::LIGHT;
    }

    return ThemeManager::ColorScheme::NO_PREFERENCE;
}

void SettingsPortal::settingsChanged(QString ns, QString key, QDBusVariant value)
{
    if (ns == "org.freedesktop.appearance") {

        if (key == "color-scheme") {
            m_colorScheme = unsignedToColorScheme(value.variant().toUInt());
            Q_EMIT colorSchemeChanged();
            return;
        }

        if (key == "contrast") {
            m_highContrast = value.variant().toBool();
            Q_EMIT highContrastChanged();
            return;
        }
    }
}
