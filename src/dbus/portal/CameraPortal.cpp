#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QLoggingCategory>

#include "CameraPortal.h"

Q_LOGGING_CATEGORY(lcCameraPortal, "gonnect.dbus.portal.Camera")

CameraPortal::CameraPortal(QObject *parent)
    : AbstractPortal{ FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                      CAMERA_PORTAL_INTERFACE, parent }
{
    m_isCameraPresent = m_interface->property("IsCameraPresent").toBool();
    qCInfo(lcCameraPortal) << "portal valid:" << isValid()
                        << "camera present:" << m_isCameraPresent;

    QDBusConnection::sessionBus().connect(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
            QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"),
            this, SLOT(propertiesChanged(QString, QVariantMap, QStringList)));
}

void CameraPortal::propertiesChanged(const QString &interfaceName, const QVariantMap &changed,
                                     const QStringList &invalidated)
{
    if (interfaceName != QLatin1String(CAMERA_PORTAL_INTERFACE)) {
        return;
    }

    bool present = false;

    if (changed.contains(QStringLiteral("IsCameraPresent"))) {
        present = changed.value(QStringLiteral("IsCameraPresent")).toBool();
    } else if (invalidated.contains(QStringLiteral("IsCameraPresent"))) {
        present = m_interface->property("IsCameraPresent").toBool();
    } else {
        return;
    }

    if (present != m_isCameraPresent) {
        m_isCameraPresent = present;
        qCInfo(lcCameraPortal) << "camera present:" << present;
        Q_EMIT isCameraPresentChanged();
    }
}

void CameraPortal::AccessCamera(PortalResponse callback)
{
    if (!isValid()) {
        callback(ResponseCode::ERROR, { { "error", tr("Camera portal is not available") } });
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(FREEDESKTOP_DBUS_PORTAL_SERVICE,
                                                          FREEDESKTOP_DBUS_PORTAL_PATH,
                                                          CAMERA_PORTAL_INTERFACE, "AccessCamera");

    const QString token = generateHandleToken();
    message << QVariantMap{ { "handle_token", QVariant(token) } };

    portalCall(token, message, [this, callback](uint code, const QVariantMap &response) {
        const bool granted = code == ResponseCode::SUCCESS;

        switch (code) {
        case ResponseCode::SUCCESS:
            qCInfo(lcCameraPortal) << "camera access granted";
            break;
        case ResponseCode::CANCELED:
            qCInfo(lcCameraPortal) << "camera access was denied by the user";
            break;
        default:
            qCWarning(lcCameraPortal)
                    << "camera access request failed:" << response.value("error").toString();
            break;
        }

        if (granted != m_hasAccess) {
            m_hasAccess = granted;
            Q_EMIT hasAccessChanged();
        }

        callback(code, response);
    });
}

void CameraPortal::OpenPipeWireRemote(std::function<void(int, const QString &)> callback)
{
    if (!isValid()) {
        callback(-1, tr("Camera portal is not available"));
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH, CAMERA_PORTAL_INTERFACE,
            "OpenPipeWireRemote");

    message << QVariantMap{};

    auto watcher =
            new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [callback](QDBusPendingCallWatcher *watcher) {
                watcher->deleteLater();

                QDBusPendingReply<QDBusUnixFileDescriptor> reply = *watcher;

                if (reply.isError()) {
                    qCWarning(lcCameraPortal)
                            << "OpenPipeWireRemote failed:" << reply.error().message();
                    callback(-1, reply.error().message());
                    return;
                }

                // QDBusUnixFileDescriptor owns its descriptor and closes it once the
                // reply goes out of scope. Duplicate it.
                const int fd = ::dup(reply.value().fileDescriptor());

                if (fd < 0) {
                    const QString error = QString::fromLocal8Bit(std::strerror(errno));
                    qCWarning(lcCameraPortal) << "could not duplicate PipeWire fd:" << error;
                    callback(-1, error);
                    return;
                }

                qCDebug(lcCameraPortal) << "opened PipeWire remote on fd" << fd;
                callback(fd, {});
            });
}
