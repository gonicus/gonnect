#include "InhibitPortal.h"

Q_LOGGING_CATEGORY(lcInhibit, "gonnect.session.inhibit")

InhibitPortal::InhibitPortal(QObject *parent)
    : AbstractPortal{ FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                      INHIBIT_PORTAL_INTERFACE, parent }
{
    // Start a monitoring session
    createMonitor([this](uint code, const QVariantMap &response) {
        switch (code) {
        case 0:
            qCInfo(lcInhibit) << "started monitoring session";
            m_session = new OrgFreedesktopPortalSessionInterface(
                    FREEDESKTOP_DBUS_PORTAL_SERVICE, response.value("session_handle").toString(),
                    QDBusConnection::sessionBus(), this);
            break;
        case 1:
            qCWarning(lcInhibit) << "monitor request was rejected by portal";
            break;
        case 2:
            qCWarning(lcInhibit) << "monitor request to portal failed";
            break;
        }
    });

    // Listen for state changes
    m_interface->connection().connect(FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                                      INHIBIT_PORTAL_INTERFACE, "StateChanged", this,
                                      SLOT(sessionStateReceived(QDBusObjectPath, QVariantMap)));
}

InhibitPortal::~InhibitPortal()
{
    release();

    if (m_session) {
        m_session->Close();
    }
}

void InhibitPortal::inhibit(unsigned flags, const QString &reason, PortalResponse callback)
{
    if (m_request) {
        qCCritical(lcInhibit) << "inhibit has already been sent";
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(FREEDESKTOP_DBUS_PORTAL_SERVICE,
                                                          FREEDESKTOP_DBUS_PORTAL_PATH,
                                                          INHIBIT_PORTAL_INTERFACE, "Inhibit");

    QString token = generateHandleToken();
    QVariantMap options = { { "handle_token", QVariant(token) }, { "reason", QVariant(reason) } };

    message << parentId() << flags << options;

    QString responsePath = portalCall(token, message, callback);
    m_request = new OrgFreedesktopPortalRequestInterface(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, responsePath, QDBusConnection::sessionBus(), this);
}

void InhibitPortal::release()
{
    // Send Close() on returned handle
    if (m_request) {
        m_request->Close();
        delete m_request;
    }

    m_request = nullptr;
}

void InhibitPortal::sessionStateReceived(const QDBusObjectPath &, const QVariantMap &map)
{
    Q_EMIT stateChanged(
            map.value("screensaver-active", false).toBool(),
            static_cast<InhibitHelper::InhibitState>(map.value("session-state").toUInt()));
}

void InhibitPortal::queryEndResponse()
{
    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH, INHIBIT_PORTAL_INTERFACE,
            "QueryEndResponse");

    message << m_session->path();

    m_interface->connection().call(message);
}

void InhibitPortal::createMonitor(PortalResponse callback)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH, INHIBIT_PORTAL_INTERFACE,
            "CreateMonitor");

    QString token = generateHandleToken();
    QString session_token = generateHandleToken();
    QVariantMap options = { { "handle_token", QVariant(token) },
                            { "session_handle_token", QVariant(session_token) } };

    message << parentId() << options;

    portalCall(token, message, callback);
}
