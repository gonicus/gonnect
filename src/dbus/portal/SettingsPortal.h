#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QColor>
#include "PortalSettings.h"

class SettingsPortal : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    Q_PROPERTY(ColorScheme colorScheme READ colorScheme NOTIFY colorSchemeChanged FINAL)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged FINAL)
    Q_PROPERTY(bool highContrast READ highContrast NOTIFY highContrastChanged FINAL)

public:
    static SettingsPortal *create(QQmlEngine *, QJSEngine *) { return &SettingsPortal::instance(); }

    static SettingsPortal &instance()
    {
        static SettingsPortal *_instance = nullptr;
        if (!_instance) {
            _instance = new SettingsPortal();
        }
        return *_instance;
    }

    enum class ColorScheme {
        NO_PREFERENCE,
        DARK,
        LIGHT,
    };
    Q_ENUM(ColorScheme)

    ColorScheme colorScheme() const { return m_colorScheme; }
    QColor accentColor() const { return m_accentColor; }
    bool highContrast() const { return m_highContrast; }

signals:
    void colorSchemeChanged();
    void accentColorChanged();
    void highContrastChanged();

private:
    SettingsPortal(QObject *parent = nullptr);

    void settingsChanged(QString ns, QString key, QDBusVariant value);

    ColorScheme unsignedToColorScheme(unsigned value);

    OrgFreedesktopPortalSettingsInterface *m_portal;

    ColorScheme m_colorScheme = ColorScheme::NO_PREFERENCE;
    QColor m_accentColor;

    bool m_highContrast = false;

    Q_DISABLE_COPY(SettingsPortal)
};
