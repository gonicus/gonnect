#include "VideoManager.h"
#include "FramePump.h"
#include "QtCameraSource.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QRegularExpression>

#include <algorithm>
#include <chrono>

#include <QCoroSignal>

#ifdef Q_OS_FLATPAK
#  include "CameraPortal.h"
#endif

#ifdef Q_OS_LINUX
#  include "linux/PipeWireCameraSource.h"
#endif

Q_LOGGING_CATEGORY(lcVideoManager, "gonnect.media.video")

namespace {

constexpr auto StallTimeout = std::chrono::seconds(10);

bool isSameDeviceList(const QList<VideoDevice> &left, const QList<VideoDevice> &right)
{
    return std::ranges::equal(left, right, [](const VideoDevice &a, const VideoDevice &b) {
        return a.id == b.id && a.description == b.description && a.isDefault == b.isDefault;
    });
}

// Two different cameras can have same name - use this to seperate them in menus
void numberDuplicateDescriptions(QList<VideoDevice> &devices)
{
    QHash<QString, int> counts;
    for (const auto &device : std::as_const(devices)) {
        ++counts[device.description];
    }

    QHash<QString, int> numbers;
    for (auto &device : devices) {
        const QString description = device.description;
        if (counts.value(description) > 1) {
            device.description =
                    QStringLiteral("%1 (%2)").arg(description).arg(++numbers[description]);
        }
    }
}

} // namespace

VideoManager::VideoManager(QObject *parent) : QObject{ parent }
{

    m_settings = std::make_unique<AppSettings>();

    m_debounceUpdateTimer.setSingleShot(true);
    m_debounceUpdateTimer.setInterval(500);
    m_debounceUpdateTimer.callOnTimeout(this, &VideoManager::updateDevices);

    m_stallTimer.setInterval(StallTimeout);
    m_stallTimer.callOnTimeout(this, &VideoManager::checkForStall);

    setCameraSource(std::make_unique<QtCameraSource>());

    connect(this, &VideoManager::selectedDeviceIdChanged, this, [this]() {
        const auto key = QString("video%1/camera").arg(m_currentProfile);
        const auto device = deviceById(m_selectedDeviceId);

        // Default device is fallback (requires no profile entry)
        if (device.isNull() || device.isDefault) {
            qCInfo(lcVideoManager) << "Video device selected:" << m_selectedDeviceId
                                   << "(default - no profile entry)";
            m_settings->remove(key);
        } else {
            qCInfo(lcVideoManager) << "Video device selected:" << m_selectedDeviceId
                                   << "saving as profile" << m_currentProfile;
            m_settings->setValue(key, m_selectedDeviceId);
        }
    });

    connect(this, &VideoManager::selectedDeviceIdChanged, this, [this]() {
        if (!m_consumers.isEmpty()) {
            startCamera();
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
    connect(m_cameraSource.get(), &CameraSource::errorOccurred, this,
            &VideoManager::reportCameraError);

    updateFrameHandler();
    if (!m_consumers.isEmpty()) {
        startCamera();
    }
}

void VideoManager::updateFrameHandler()
{
    // Must copy consumer list because running on camera's thread
    m_cameraSource->setFrameHandler([this, consumers = m_consumers](const QVideoFrame &frame) {
        m_hasReceivedFrame.store(true, std::memory_order_relaxed);

        for (auto *consumer : consumers) {
            consumer->push(frame);
        }
    });
}

void VideoManager::startCamera()
{
    setCameraError({});
    m_hasReceivedFrame = false;
    m_stallTimer.start();
    m_cameraSource->start(m_selectedDeviceId);
}

void VideoManager::stopCamera()
{
    m_stallTimer.stop();
    m_cameraSource->stop();
    setCameraError({});
}

void VideoManager::checkForStall()
{
    // Camera can hang without reporting error
    if (m_hasReceivedFrame.exchange(false)) {
        return;
    }

    reportCameraError(tr("Camera does not deliver any images"));
}

void VideoManager::reportCameraError(const QString &message)
{
    m_stallTimer.stop();

    qCWarning(lcVideoManager) << "Camera error:" << message;
    setCameraError(message);
    Q_EMIT cameraErrorOccurred(message);
}

void VideoManager::setCameraError(const QString &message)
{
    if (m_cameraError != message) {
        m_cameraError = message;
        Q_EMIT cameraErrorChanged();
    }
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
        startCamera();
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
        stopCamera();
    }
}

void VideoManager::updateDevices()
{
    QList<VideoDevice> devices = m_cameraSource->devices();
    numberDuplicateDescriptions(devices);

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
