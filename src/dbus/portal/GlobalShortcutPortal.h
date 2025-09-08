#pragma once

#include <QMap>
#include <QString>
#include "AbstractPortal.h"
#include "GlobalShortcuts.h"

#define GLOBALSHORTCUT_PORTAL_INTERFACE "org.freedesktop.portal.GlobalShortcuts"

Q_DECLARE_METATYPE(Shortcuts)

class GlobalShortcutPortal : public AbstractPortal
{
    Q_OBJECT

public:
    explicit GlobalShortcutPortal(QObject *parent = nullptr);
    ~GlobalShortcutPortal();

    void initialize();
    bool isSupported() const { return m_supported; }

    void setShortcuts(Shortcuts &shortcuts) { m_shortcuts = shortcuts; }
    QList<ShortcutItem *> shortcuts() const { return m_currentShortcuts; }

public slots:
    void shortcutActivatedReceived(const QDBusObjectPath &session_handle, const QString &id,
                                   qulonglong timestamp, const QVariantMap &map);
    void shortcutsChangedReceived(const QDBusObjectPath &session_handle,
                                  const Shortcuts &shortcuts);

signals:
    void initialized();
    void shortcutsChanged();
    void activated(QString id);

private:
    void checkShortcuts();
    void registerShortcuts();
    void updateShortcuts(const Shortcuts &shortcuts);

    void createSession(PortalResponse callback);
    void bindShortcuts(PortalResponse callback);
    void listShortcuts(PortalResponse callback);

    Shortcuts m_shortcuts;
    QList<ShortcutItem *> m_currentShortcuts;

    bool m_supported = false;
};
