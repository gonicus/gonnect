#include "VideoManager.h"
#include "QtCameraSource.h"

#include <QLoggingCategory>
#include <QRegularExpression>

#include <algorithm>

#ifdef Q_OS_FLATPAK
#  include "CameraPortal.h"
#endif

#ifdef Q_OS_LINUX
#  include "linux/PipeWireCameraSource.h"
#endif

Q_LOGGING_CATEGORY(lcVideoManager, "gonnect.sip.video")

namespace {

bool isSameDeviceList(const QList<VideoDevice> &left, const QList<VideoDevice> &right)
{
    return std::ranges::equal(left, right, [](const VideoDevice &a, const VideoDevice &b) {
        return a.id == b.id && a.description == b.description && a.isDefault == b.isDefault;
    });
}
} // namespace

VideoManager::VideoManager(QObject *parent) : QObject{ parent }
{

    m_settings = std::make_unique<AppSettings>();

    m_debounceUpdateTimer.setSingleShot(true);
    m_debounceUpdateTimer.setInterval(500);
    m_debounceUpdateTimer.callOnTimeout(this, &VideoManager::updateDevices);

    setCameraSource(std::make_unique<QtCameraSource>());

    connect(this, &VideoManager::selectedDeviceIdChanged, this, [this]() {
        qCInfo(lcVideoManager) << "Video device selected:" << m_selectedDeviceId
                               << "saving as profile" << m_currentProfile;

        // Save selected device to settings
        auto device = deviceById(m_selectedDeviceId);
        if (device.isNull() || device.isDefault) {
            m_settings->remove(QString("video%1/camera").arg(m_currentProfile));
        } else {
            m_settings->setValue(QString("video%1/camera").arg(m_currentProfile),
                                 m_selectedDeviceId);
        }
    });

    connect(this, &VideoManager::devicesChanged, this, &VideoManager::updateProfile);

    updateDevices();
}

void VideoManager::setCameraSource(std::unique_ptr<CameraSource> source)
{
    m_cameraSource = std::move(source);
    connect(m_cameraSource.get(), &CameraSource::devicesChanged, this,
            [this]() { m_debounceUpdateTimer.start(); });
}

void VideoManager::acquireCameras()
{
#ifdef Q_OS_FLATPAK
    auto &portal = CameraPortal::instance();

    if (qobject_cast<PipeWireCameraSource *>(m_cameraSource.get()) || m_portalPending) {
        return;
    }

    if (!portal.isValid()) {
        qCInfo(lcVideoManager) << "camera portal unavailable - falling back to QMediaDevices";
        setCameraAccessState(CameraAccessState::Granted);
        return;
    }

    setCameraAccessState(CameraAccessState::Pending);
    m_portalPending = true;

    portal.AccessCamera([this, &portal](uint code, const QVariantMap &response) {
        if (code != AbstractPortal::ResponseCode::SUCCESS) {
            m_portalPending = false;
            m_settings->setValue("video/cameraAccessDenied", true);

            const bool isPipeWireCameraActive =
                    qgetenv("QTWEBENGINE_CHROMIUM_FLAGS").contains("WebRtcPipeWireCamera");
            setCameraAccessState(isPipeWireCameraActive ? CameraAccessState::DeniedRestartRequired
                                                        : CameraAccessState::Denied);

            qCWarning(lcVideoManager)
                    << "camera access not granted:" << response.value("error").toString();
            return;
        }

        m_settings->remove("video/cameraAccessDenied");
        setCameraAccessState(CameraAccessState::Granted);

        portal.OpenPipeWireRemote([this](int fd, const QString &error) {
            m_portalPending = false;

            if (fd < 0) {
                qCWarning(lcVideoManager) << "could not open PipeWire remote:" << error;
                return;
            }

            auto source = std::make_unique<PipeWireCameraSource>();
            if (!source->open(fd)) {
                qCWarning(lcVideoManager) << "Cannot connect to PipeWire camera remove - "
                                             "staying with Qt cameras";
                return;
            }

            setCameraSource(std::move(source));
            m_debounceUpdateTimer.start();
        });
    });
#elif defined(Q_OS_LINUX)
    setCameraAccessState(CameraAccessState::Granted);

    if (qobject_cast<PipeWireCameraSource *>(m_cameraSource.get())) {
        return;
    }

    auto source = std::make_unique<PipeWireCameraSource>();
    if (!source->open(-1)) {
        qCWarning(lcVideoManager) << "Cannot connect to PipeWire - staying with Qt cameras";
        return;
    }

    setCameraSource(std::move(source));
    m_debounceUpdateTimer.start();
#else
    setCameraAccessState(CameraAccessState::Granted);
#endif
}

bool VideoManager::isDeviceAvailable(const QString &deviceId) const
{
    if (deviceId.isEmpty()) {
        return true; // Default device
    }

    for (const auto &dev : std::as_const(m_devices)) {
        if (!dev.isNull() && dev.id == deviceId) {
            return true;
        }
    }

    return false;
}

VideoDevice VideoManager::deviceById(const QString &deviceId) const
{
    for (const auto &dev : std::as_const(m_devices)) {
        if (dev.id == deviceId) {
            return dev;
        }
    }

    return VideoDevice();
}

VideoDevice VideoManager::selectedDevice() const
{
    if (m_selectedDeviceId.isEmpty()) {
        return VideoDevice();
    }

    return deviceById(m_selectedDeviceId);
}

bool VideoManager::isCameraAccessDenied() const
{
    return m_cameraAccessState == CameraAccessState::Denied
            || m_cameraAccessState == CameraAccessState::DeniedRestartRequired;
}

void VideoManager::setCameraAccessState(CameraAccessState value)
{
    if (m_cameraAccessState != value) {
        m_cameraAccessState = value;
        Q_EMIT cameraAccessStateChanged();
    }
}

void VideoManager::updateDevices()
{
    QList<VideoDevice> devices = m_cameraSource->devices();

    if (isSameDeviceList(devices, m_devices)) {
        return;
    }

    m_devices = devices;

    qCInfo(lcVideoManager) << "Found" << m_devices.size() << "video input devices";

    for (const auto &device : std::as_const(m_devices)) {
        qCInfo(lcVideoManager) << "  " << device.id << device.description
                               << "default:" << (device.isDefault ? "yes" : "no");
    }

    Q_EMIT devicesChanged();
}

void VideoManager::updateProfile()
{
    m_currentProfile = 0;

    static QRegularExpression isVideoProfile = QRegularExpression("^video[0-9]+$");
    const QStringList groups = m_settings->childGroups();
    QString videoId;

    for (const auto &group : groups) {
        if (isVideoProfile.match(group).hasMatch()) {
            videoId = m_settings->value(QString("%1/camera").arg(group)).toString();

            if (isDeviceAvailable(videoId)) {
                setProperty("selectedDeviceId", videoId);
                return;
            }

            ++m_currentProfile;
        }
    }

    // Select default device as fallback
    for (const auto &device : std::as_const(m_devices)) {
        if (device.isDefault) {
            setProperty("selectedDeviceId", device.id);
            return;
        }
    }
}
