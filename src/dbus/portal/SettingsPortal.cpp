#include <QDBusConnection>
#include "SettingsPortal.h"

Q_LOGGING_CATEGORY(lcSettingsPortal, "gonnect.dbus.portal.Settings")

QDBusArgument &operator<<(QDBusArgument &arg, const RgbColor &c)
{
    arg.beginStructure();
    arg << c.r << c.g << c.b;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator>>(const QDBusArgument &arg, RgbColor &c)
{
    arg.beginStructure();
    arg >> c.r >> c.g >> c.b;
    arg.endStructure();
    return const_cast<QDBusArgument &>(arg);
}

SettingsPortal::SettingsPortal(QObject *parent) : QObject(parent)
{
    static bool registered = false;
    if (!registered) {
        qDBusRegisterMetaType<RgbColor>();
        registered = true;
    }

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

        reply = m_portal->ReadOne("org.freedesktop.appearance", "accent-color");
        reply.waitForFinished();
        if (reply.isValid()) {
            m_accentColor = dbusDoubleTripleToColor(reply.value());
            Q_EMIT accentColorChanged();
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

        if (key == "accent-color") {
            m_accentColor = dbusDoubleTripleToColor(value);
            Q_EMIT accentColorChanged();
            return;
        }
    }
}

QColor SettingsPortal::dbusDoubleTripleToColor(QDBusVariant value)
{
    const QVariant v = value.variant();

    if (v.canConvert<QDBusArgument>()) {
        QString sig = v.value<QDBusArgument>().currentSignature();
        if (sig != QLatin1String("(ddd)")) {
            qCWarning(lcSettingsPortal) << "color struct is not (ddd)";
            return QColor();
        }
    }

    RgbColor c = qdbus_cast<RgbColor>(v);
    return QColor::fromRgbF(c.r, c.g, c.b);
}
