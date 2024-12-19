#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include "BackgroundPortal.h"

BackgroundPortal::BackgroundPortal(QObject *parent)
    : AbstractPortal{ FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                      BACKGROUND_PORTAL_INTERFACE, parent }
{
}

void BackgroundPortal::RequestBackground(bool autostart, bool useDBusActivation,
                                         PortalResponse callback)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
            BACKGROUND_PORTAL_INTERFACE, "RequestBackground");

    QString token = generateHandleToken();

    QVariantMap options = {
        { "handle_token", QVariant(token) },
        { "autostart", QVariant(autostart) },
        { "dbus-activatable", QVariant(useDBusActivation) },
        { "reason",
          QVariant(QString(tr(
                  "Don't miss any calls by automatically starting GOnnect on session start."))) }
    };

    message << parentId() << options;

    portalCall(token, message, callback);
}

void BackgroundPortal::SetStatus(const QString &msg)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
            "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.Background", "SetStatus");

    QVariantMap options = { { "message", QVariant(msg) } };

    message << options;

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);

    auto watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [](QDBusPendingCallWatcher *watcher) {
                watcher->deleteLater();
                QDBusPendingReply<QDBusObjectPath> reply = *watcher;
                if (reply.isError()) {
                    qWarning("DBus call for SetStatus failed: %s",
                             qPrintable(reply.error().message()));
                }
            });
}
