#include "AppSettings.h"
#include "GlobalShortcutPortal.h"

Q_LOGGING_CATEGORY(lcShortcuts, "gonnect.session.shortcuts")

GlobalShortcutPortal::GlobalShortcutPortal(QObject *parent)
    : AbstractPortal{ FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                      GLOBALSHORTCUT_PORTAL_INTERFACE, parent }
{
    // List of supported shortcuts
    m_shortcuts = {
        { "dial",
          { { "description", tr("Show dial window and focus search field") },
            { "preferred_trigger", "CTRL+ALT+K" } } },
        { "hangup",
          { { "description", tr("End all calls") }, { "preferred_trigger", "CTRL+ALT+E" } } },
        { "redial",
          { { "description", tr("Redial last outgoing call") },
            { "preferred_trigger", "CTRL+ALT+R" } } },
        { "toggle-hold",
          { { "description", tr("Toggle hold") }, { "preferred_trigger", "CTRL+ALT+M" } } },
    };
}

void GlobalShortcutPortal::initialize()
{
    createSession([this](uint code, const QVariantMap &response) {
        switch (code) {
        case 0:
            qCInfo(lcShortcuts) << "started global shortcut session";
            m_session = new OrgFreedesktopPortalSessionInterface(
                    FREEDESKTOP_DBUS_PORTAL_SERVICE, response.value("session_handle").toString(),
                    QDBusConnection::sessionBus(), this);

            checkShortcuts();

            // Listen for state changes
            m_interface->connection().connect(
                    FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                    GLOBALSHORTCUT_PORTAL_INTERFACE, "Activated", this,
                    SLOT(shortcutActivatedReceived(QDBusObjectPath, QString, qulonglong,
                                                   QVariantMap)));

            m_interface->connection().connect(
                    FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                    GLOBALSHORTCUT_PORTAL_INTERFACE, "ShortcutsChanged", this,
                    SLOT(shortcutsChangedReceived(QDBusObjectPath, Shortcuts)));

            m_supported = true;
            break;
        case 1:
            qCWarning(lcShortcuts) << "session create request was rejected by portal"
                                   << response.value("error").toString();
            break;
        case 2:
            qCWarning(lcShortcuts) << "session create request to portal failed:"
                                   << response.value("error").toString();
            break;
        }
    });

    emit initialized();
}

void GlobalShortcutPortal::registerShortcuts()
{
    bindShortcuts([this](uint code, const QVariantMap &response) {
        if (code == 0) {
            if (response.contains("shortcuts")) {
                updateShortcuts(
                        qdbus_cast<Shortcuts>(response.value("shortcuts").value<QDBusArgument>()));
            } else {
                qCWarning(lcShortcuts) << "no shortcuts registered";
            }
        } else {
            qCCritical(lcShortcuts) << "failed to bind shortcuts:" << code;
        }
    });
}

void GlobalShortcutPortal::checkShortcuts()
{
    listShortcuts([this](uint code, const QVariantMap &response) {
        if (code == 0) {
            if (response.contains("shortcuts")) {
                auto sc = qdbus_cast<Shortcuts>(response.value("shortcuts").value<QDBusArgument>());

                // Check if we have new key bindings available
                QStringList sourceKeys, targetKeys;
                for (auto &el : std::as_const(sc)) {
                    sourceKeys.push_back(el.first);
                }
                std::sort(sourceKeys.begin(), sourceKeys.end());

                for (auto &el : std::as_const(m_shortcuts)) {
                    targetKeys.push_back(el.first);
                }
                std::sort(targetKeys.begin(), targetKeys.end());

                // Same keys - take the upstream list to have the updated bindings
                if (sourceKeys.count() != 0 && sourceKeys == targetKeys) {
                    updateShortcuts(sc);
                } else {
                    registerShortcuts();
                }

            } else {
                qCWarning(lcShortcuts) << "no shortcuts registered";
            }
        } else {
            qCCritical(lcShortcuts) << "failed to list shortcuts:" << code;
        }
    });
}

void GlobalShortcutPortal::listShortcuts(PortalResponse callback)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
            GLOBALSHORTCUT_PORTAL_INTERFACE, "ListShortcuts");

    QString token = generateHandleToken();

    QVariantMap options = { { "handle_token", QVariant(token) } };

    message << QDBusObjectPath(m_session->path()) << options;

    portalCall(token, message, callback);
}

void GlobalShortcutPortal::bindShortcuts(PortalResponse callback)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
            GLOBALSHORTCUT_PORTAL_INTERFACE, "BindShortcuts");

    QString token = generateHandleToken();

    QVariantMap options = { { "handle_token", QVariant(token) } };

    message << QDBusObjectPath(m_session->path()) << QVariant::fromValue(m_shortcuts) << parentId()
            << options;

    portalCall(token, message, callback);
}

void GlobalShortcutPortal::shortcutActivatedReceived(const QDBusObjectPath &, const QString &id,
                                                     qulonglong, const QVariantMap &)
{
    emit activated(id);
}

void GlobalShortcutPortal::shortcutsChangedReceived(const QDBusObjectPath &,
                                                    const Shortcuts &shortcuts)
{
    updateShortcuts(shortcuts);
}

void GlobalShortcutPortal::updateShortcuts(const Shortcuts &shortcuts)
{
    qDeleteAll(m_currentShortcuts);
    m_currentShortcuts.clear();

    for (auto &sc : std::as_const(shortcuts)) {
        auto sci = new ShortcutItem();
        sci->id = sc.first;
        sci->description = sc.second.value("description", "Error: no description").toString();
        sci->triggerDescription =
                sc.second.value("trigger_description", "Error: no trigger description").toString();
        m_currentShortcuts.push_back(sci);
    }

    emit shortcutsChanged();
}

void GlobalShortcutPortal::createSession(PortalResponse callback)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
            GLOBALSHORTCUT_PORTAL_INTERFACE, "CreateSession");

    AppSettings settings;
    QString sessionToken;
    bool hasToken = settings.contains("session/shortcuts");
    if (hasToken) {
        sessionToken = settings.value("session/shortcuts").toString();
    } else {
        sessionToken = generateHandleToken();
        settings.setValue("session/shortcuts", sessionToken);
    }

    QString token = generateHandleToken();
    QVariantMap options = { { "handle_token", QVariant(token) },
                            { "session_handle_token", QVariant(sessionToken) } };

    message << options;

    portalCall(token, message, callback);
}

GlobalShortcutPortal::~GlobalShortcutPortal()
{
    if (m_session) {
        m_session->Close();
    }

    qDeleteAll(m_currentShortcuts);
    m_currentShortcuts.clear();
}
