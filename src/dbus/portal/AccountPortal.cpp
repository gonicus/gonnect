#include "AccountPortal.h"

AccountPortal::AccountPortal(QObject *parent)
    : AbstractPortal{ FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                      ACCOUNT_PORTAL_INTERFACE, parent }
{
}

void AccountPortal::GetUserInformation(const QString &reason, PortalResponse callback)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH, ACCOUNT_PORTAL_INTERFACE,
            "GetUserInformation");

    QString token = generateHandleToken();

    QVariantMap options = { { "handle_token", QVariant(token) }, { "reason", reason } };

    message << parentId() << options;

    portalCall(token, message, callback);
}
