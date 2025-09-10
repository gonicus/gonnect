#include "VideoManager.h"

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(lcVideoManager, "gonnect.sip.video")

VideoManager::VideoManager(QObject *parent) : QObject{ parent }
{

    m_settings = std::make_unique<AppSettings>();

    m_debounceUpdateTimer.setSingleShot(true);
    m_debounceUpdateTimer.setInterval(500);
    m_debounceUpdateTimer.callOnTimeout(this, &VideoManager::updateDevices);

    connect(&m_mediaDevices, &QMediaDevices::videoInputsChanged, this,
            [this]() { m_debounceUpdateTimer.start(); });

    connect(this, &VideoManager::selectedDeviceIdChanged, this, [this]() {
        qCInfo(lcVideoManager) << "Video device selected:" << m_selectedDeviceId
                               << "saving as profile" << m_currentProfile;

        // Save selected device to settings
        auto device = deviceById(m_selectedDeviceId);
        if (device.isNull() || device.isDefault()) {
            m_settings->remove(QString("video%1/camera").arg(m_currentProfile));
        } else {
            m_settings->setValue(QString("video%1/camera").arg(m_currentProfile),
                                 m_selectedDeviceId);
        }
    });

    connect(this, &VideoManager::devicesChanged, this, &VideoManager::updateProfile);

    updateDevices();
}

bool VideoManager::isDeviceAvailable(const QString &deviceId) const
{
    if (deviceId.isEmpty()) {
        return true; // Default device
    }

    for (const auto &dev : std::as_const(m_devices)) {
        if (!dev.isNull() && dev.id() == deviceId) {
            return true;
        }
    }

    return false;
}

QCameraDevice VideoManager::deviceById(const QString &deviceId) const
{
    for (const auto &dev : std::as_const(m_devices)) {
        if (dev.id() == deviceId) {
            return dev;
        }
    }

    return QCameraDevice();
}

QCameraDevice VideoManager::selectedDevice() const
{
    if (m_selectedDeviceId.isEmpty()) {
        return QCameraDevice();
    }

    return deviceById(m_selectedDeviceId);
}

void VideoManager::updateDevices()
{
    m_devices.clear();

    const auto videoInputs = m_mediaDevices.videoInputs();
    qCInfo(lcVideoManager) << "Found" << videoInputs.size() << "video input devices";

    for (const auto &device : videoInputs) {
        qCInfo(lcVideoManager) << "  " << device.id() << device.description()
                               << "default:" << (device.isDefault() ? "yes" : "no")
                               << "valid:" << (device.isNull() ? "no" : "yes");

        if (!device.isNull()) {
            m_devices.append(device);
        }
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
        if (device.isDefault()) {
            setProperty("selectedDeviceId", device.id());
            Q_EMIT selectedDeviceIdChanged();
            return;
        }
    }
}
