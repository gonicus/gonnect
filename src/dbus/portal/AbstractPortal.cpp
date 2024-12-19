#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QRandomGenerator>
#include <private/qgenericunixservices_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include "AbstractPortal.h"
#include "Application.h"

PortalResponseTracker::PortalResponseTracker(PortalResponse callback, QObject *parent)
    : QObject(parent), m_callback(callback)
{
}

void PortalResponseTracker::responseReceived(uint state, const QVariantMap &map)
{
    m_callback(state, map);
    deleteLater();
}

AbstractPortal::AbstractPortal(const QString &service, const QString &path,
                               const QString &interface, QObject *parent)
    : QObject(parent)
{
    m_interface = new QDBusInterface(service, path, interface);
    m_interface->setParent(this);
}

QString AbstractPortal::replyPath(const QString &token) const
{
    /* Since version 0.9 of xdg-desktop-portal, the handle will be of the form

        /org/freedesktop/portal/desktop/request/SENDER/TOKEN

       where ``SENDER`` is the callers unique name, with the initial ``':'`` removed and
       all ``'.'`` replaced by ``'_'``, and ``TOKEN`` is a unique token that the caller provided
       with the handle_token key in the options vardict.
    */
    QString uniqueName = QDBusConnection::sessionBus().baseService().replace(".", "_").sliced(1);

    return "/org/freedesktop/portal/desktop/request/" + uniqueName + "/" + token;
}

QString AbstractPortal::portalCall(const QString &handle, const QDBusMessage &message,
                                   PortalResponse callback)
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        callback(ResponseCode::ERROR,
                 { { "error", tr("No DBus session bus connection available") } });
        return "";
    }

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);

    // Since xdg-desktop-portal v0.9 the reply path is predictable. Subscribe in advance
    // to avoid race conditions and check if we're compatible later on.
    QString preCalculatedReplyPath = replyPath(handle);
    auto tracker = new PortalResponseTracker(callback, this);

    QDBusConnection::sessionBus().connect("org.freedesktop.portal.Desktop", preCalculatedReplyPath,
                                          "org.freedesktop.portal.Request", "Response", tracker,
                                          SLOT(responseReceived(uint, QVariantMap)));

    auto watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [preCalculatedReplyPath, tracker, callback, handle](QDBusPendingCallWatcher *watcher) {
                watcher->deleteLater();
                QDBusPendingReply<QDBusObjectPath> reply = *watcher;
                if (reply.isError()) {
                    callback(ResponseCode::ERROR,
                             { { "error",
                                 tr("DBus call failed: %1")
                                         .arg(qPrintable(reply.error().message())) } });
                } else {
                    auto replyPath = reply.value().path();

                    // We're not v0.9+, so re connect with the response/reply path
                    if (replyPath != preCalculatedReplyPath) {
                        QDBusConnection::sessionBus().disconnect(
                                "org.freedesktop.portal.Desktop", preCalculatedReplyPath,
                                "org.freedesktop.portal.Request", "Response", tracker,
                                SLOT(responseReceived(uint, QVariantMap)));

                        QDBusConnection::sessionBus().connect(
                                "org.freedesktop.portal.Desktop", replyPath,
                                "org.freedesktop.portal.Request", "Response", tracker,
                                SLOT(responseReceived(uint, QVariantMap)));
                    }
                }
            });

    return preCalculatedReplyPath;
}

QString AbstractPortal::generateHandleToken() const
{
    return QString::number(QRandomGenerator::global()->bounded(0, 100000));
}

QString AbstractPortal::parentId() const
{
    QString parentId;

    auto unixServices = dynamic_cast<QGenericUnixServices *>(
            QGuiApplicationPrivate::platformIntegration()->services());

    auto rootWindow = qobject_cast<Application *>(qGuiApp)->rootWindow();
    if (rootWindow && unixServices) {
        parentId = unixServices->portalWindowIdentifier(rootWindow);
    }

    return parentId;
}
