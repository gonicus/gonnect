#include "VideoManager.h"

#include <QLoggingCategory>
#include <QRegularExpression>

#ifdef Q_OS_FLATPAK
#  include "CameraPortal.h"
#endif

Q_LOGGING_CATEGORY(lcVideoManager, "gonnect.sip.video")

VideoManager::VideoManager(QObject *parent) : QObject{ parent }
{

    m_settings = std::make_unique<AppSettings>();

    m_debounceUpdateTimer.setSingleShot(true);
    m_debounceUpdateTimer.setInterval(500);
    m_debounceUpdateTimer.callOnTimeout(this, &VideoManager::updateDevices);

    connect(&m_mediaDevices, &QMediaDevices::videoInputsChanged, this,
            [this]() { m_debounceUpdateTimer.start(); });

#ifdef Q_OS_FLATPAK
    connect(&m_pipewireCameras, &PipeWireCameraDevices::devicesChanged, this,
            [this](){ m_debounceUpdateTimer.start(); });
#endif

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

void VideoManager::acquireCameras()
{
#ifdef Q_OS_FLATPAK
    auto &portal = CameraPortal::instance();

    if (m_pipewireCameras.isRunning() || m_portalPending) {
        return;
    }

    if (!portal.isValid() || !portal.isCameraPresent()) {
        qCInfo(lcVideoManager)
                << "camera portal unavailable or no camera present - falling back to QMediaDevices";
        return;
    }

    portal.AccessCamera([this, &portal](uint code, const QVariantMap &response) {
        if (code != AbstractPortal::ResponseCode::SUCCESS) {
            qCInfo(lcVideoManager)
                    << "camera access not granted:" << response.value("error").toString();
            return;
        }

        m_portalPending = true;

        portal.OpenPipeWireRemote([this](int fd, const QString &error) {
            m_portalPending = false;

            if (fd < 0) {
                qCWarning(lcVideoManager) << "could not open PipeWire remote:" << error;
                return;
            }

            if (!m_pipewireCameras.start(fd)) {
                qCWarning(lcVideoManager) << "could not start PipeWire camera enumeration - "
                                             "falling back to QMediaDevices";
                return;
            }

            m_debounceUpdateTimer.start();
        });
    });
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

QList<VideoDevice> VideoManager::devicesFromMediaDevices() const
{
    QList<VideoDevice> devices;

    const auto videoInputs = m_mediaDevices.videoInputs();

    for (const auto &device : videoInputs) {
        if (!device.isNull()) {
            devices.append(VideoDevice{ device.id(), device.description(), device.isDefault() });
        }
    }

    return devices;
}

void VideoManager::updateDevices()
{
    m_devices.clear();

#ifdef Q_OS_FLATPAK
    if (m_pipewireCameras.isRunning()) {
        const auto cameras = m_pipewireCameras.devices();

        for (const auto &camera : std::as_const(cameras)) {
            m_devices.append(VideoDevice{ camera.name, camera.description, false });
        }

        if (!m_devices.isEmpty()) {
            m_devices.first().isDefault = true;
        }
    } else {
        m_devices = devicesFromMediaDevices();
    }
#else
    m_devices = devicesFromMediaDevices();
#endif

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
