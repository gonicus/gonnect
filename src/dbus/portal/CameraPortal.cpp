#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QLoggingCategory>
#include <QCoroDBus>

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

QCoro::Task<AbstractPortal::Response> CameraPortal::AccessCamera()
{
    if (!isValid()) {
        co_return Response{ ResponseCode::ERROR,
                            { { "error", tr("Camera portal is not available") } } };
    }

    QDBusMessage message = QDBusMessage::createMethodCall(FREEDESKTOP_DBUS_PORTAL_SERVICE,
                                                          FREEDESKTOP_DBUS_PORTAL_PATH,
                                                          CAMERA_PORTAL_INTERFACE, "AccessCamera");

    const QString token = generateHandleToken();
    message << QVariantMap{ { "handle_token", QVariant(token) } };

    const Response response = co_await portalCallAsync(token, message);
    switch (response.code) {
    case ResponseCode::SUCCESS:
        qCInfo(lcCameraPortal) << "Camera access granted";
        break;
    case ResponseCode::CANCELED:
        qCInfo(lcCameraPortal) << "Camera access was denied by the user";
        break;
    default:
        qCWarning(lcCameraPortal) << "Camera access request failed:"
                                  << response.results.value("error").toString();
        break;
    }

    const bool granted = response.code == ResponseCode::SUCCESS;
    if (granted != m_hasAccess) {
        m_hasAccess = granted;
        Q_EMIT hasAccessChanged();
    }

    co_return response;
}

QCoro::Task<int> CameraPortal::OpenPipeWireRemote()
{
    if (!isValid()) {
        qCWarning(lcCameraPortal) << "Camera portal is not available";
        co_return -1;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH, CAMERA_PORTAL_INTERFACE,
            "OpenPipeWireRemote");

    message << QVariantMap{};

    QDBusPendingReply<QDBusUnixFileDescriptor> reply =
            QDBusConnection::sessionBus().asyncCall(message);
    co_await reply;

    if (reply.isError()) {
        qCWarning(lcCameraPortal) << "OpenPipeWireRemote failed:" << reply.error().message();
        co_return -1;
    }

    // QDBusUnixFileDescriptor owns its descriptor and closes it once the
    // reply goes out of scope. Duplicate it.
    const int fd = ::dup(reply.value().fileDescriptor());

    if (fd < 0) {
        qCWarning(lcCameraPortal) << "Could not duplicate PipeWire fd:" << std::strerror(errno);
        co_return -1;
    }

    qCDebug(lcCameraPortal) << "Opened PipeWire remote on fd" << fd;
    co_return fd;
}
