#include "VideoManager.h"
#include "FramePump.h"
#include "QtCameraSource.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QRegularExpression>

#include <algorithm>

#include <QCoroSignal>

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

    connect(this, &VideoManager::selectedDeviceIdChanged, this, [this]() {
        if (!m_consumers.isEmpty()) {
            m_cameraSource->start(m_selectedDeviceId);
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

    updateFrameHandler();
    if (!m_consumers.isEmpty()) {
        m_cameraSource->start(m_selectedDeviceId);
    }
}

void VideoManager::updateFrameHandler()
{
    // Must copy consumer list because running on camera's thread
    m_cameraSource->setFrameHandler([consumers = m_consumers](const QVideoFrame &frame) {
        for (auto *consumer : consumers) {
            consumer->push(frame);
        }
    });
}

void VideoManager::acquireCameras()
{
#ifdef Q_OS_FLATPAK
    if (m_cameraAccessState == CameraAccessState::Pending
        || m_cameraAccessState == CameraAccessState::Granted) {
        return;
    }

    auto &portal = CameraPortal::instance();
    if (!portal.isValid()) {
        qCInfo(lcVideoManager) << "camera portal unavailable - staying with Qt cameras";
        setCameraAccessState(CameraAccessState::Granted);
        return;
    }

    setCameraAccessState(CameraAccessState::Pending);
    requestCameraAccess();
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

#ifdef Q_OS_FLATPAK
QCoro::Task<> VideoManager::requestCameraAccess()
{
    // Make sure dialog is shown when window focused
    if (!qGuiApp->focusWindow()) {
        qCInfo(lcVideoManager) << "Waiting for focus to request camera access";
        co_await qCoro(qGuiApp, &QGuiApplication::focusWindowChanged);
    }

    auto &portal = CameraPortal::instance();

    const AbstractPortal::Response access = co_await portal.AccessCamera();
    if (access.code != AbstractPortal::ResponseCode::SUCCESS) {
        setCameraAccessState(CameraAccessState::Denied);
        co_return;
    }

    setCameraAccessState(CameraAccessState::Granted);

    // No open() with negative fd - would connect to PipeWire directly
    const int fd = co_await portal.OpenPipeWireRemote();
    auto source = std::make_unique<PipeWireCameraSource>();
    if (fd < 0 || !source->open(fd)) {
        qCWarning(lcVideoManager) << "Cannot connect to camera portal - staying with Qt cameras";
        co_return;
    }

    setCameraSource(std::move(source));
    m_debounceUpdateTimer.start();
}
#endif

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
    return m_cameraAccessState == CameraAccessState::Denied;
}

bool VideoManager::isCameraAvailable() const
{
    return !isCameraAccessDenied() && !m_devices.isEmpty();
}

void VideoManager::setCameraAccessState(CameraAccessState value)
{
    if (m_cameraAccessState != value) {
        m_cameraAccessState = value;
        Q_EMIT cameraAccessStateChanged();
    }
}

void VideoManager::addConsumer(FramePump *consumer)
{
    if (!consumer) {
        qCWarning(lcVideoManager) << "Ignoring nullptr consumer";
        return;
    }

    if (m_consumers.contains(consumer)) {
        return;
    }

    m_consumers.append(consumer);
    updateFrameHandler();

    if (m_consumers.size() == 1) {
        m_cameraSource->start(m_selectedDeviceId);
    }
}

void VideoManager::removeConsumer(FramePump *consumer)
{
    if (!consumer) {
        qCWarning(lcVideoManager) << "Ignoring nullptr consumer";
        return;
    }

    if (!m_consumers.removeOne(consumer)) {
        return;
    }

    updateFrameHandler();

    if (m_consumers.isEmpty()) {
        m_cameraSource->stop();
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
