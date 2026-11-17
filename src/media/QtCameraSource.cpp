#include "QtCameraSource.h"

#include <QLoggingCategory>

#include <algorithm>

Q_LOGGING_CATEGORY(lcQtCameraSource, "gonnect.media.camera.qt")

namespace {

constexpr QSize MaximumResolution(1280, 720);
constexpr float MinimumFluentFrameRate = 25.0f;

bool isBetterFormat(const QCameraFormat &candidate, const QCameraFormat &current)
{
    // fluent > large resolution > small resolution
    const bool candidateIsFluent = candidate.maxFrameRate() >= MinimumFluentFrameRate;
    const bool currentIsFluent = current.maxFrameRate() >= MinimumFluentFrameRate;
    if (candidateIsFluent != currentIsFluent) {
        return candidateIsFluent;
    }

    const auto area = [](const QCameraFormat &format) {
        return format.resolution().width() * format.resolution().height();
    };
    return area(candidate) > area(current);
}

QCameraFormat bestFormat(const QCameraDevice &device)
{
    QCameraFormat best;

    const auto formats = device.videoFormats();
    for (const auto &format : formats) {
        const QSize resolution(format.resolution());
        if (resolution.width() > MaximumResolution.width()
            || resolution.height() > MaximumResolution.height()) {
            continue;
        }

        if (best.isNull() || isBetterFormat(format, best)) {
            best = format;
        }
    }
    return best;
}

QString formatToString(const QCameraFormat &format)
{
    if (format.isNull()) {
        return QString();
    }

    return QStringLiteral("%1 %2x%3 @ %4 fps")
            .arg(QVideoFrameFormat::pixelFormatToString(format.pixelFormat()))
            .arg(format.resolution().width())
            .arg(format.resolution().height())
            .arg(format.maxFrameRate(), 0, 'f', 1);
}
} // namespace

QtCameraSource::QtCameraSource(QObject *parent) : CameraSource{ parent }
{
    m_captureSession.setVideoSink(&m_videoSink);

    connect(&m_mediaDevices, &QMediaDevices::videoInputsChanged, this,
            &CameraSource::devicesChanged);
    connect(&m_videoSink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) { deliverFrame(frame); });
}

QtCameraSource::~QtCameraSource()
{
    QtCameraSource::stop();
}

QList<VideoDevice> QtCameraSource::devices() const
{
    QList<VideoDevice> devices;

    const auto videoInputs = m_mediaDevices.videoInputs();
    for (const auto &device : videoInputs) {
        if (!device.isNull()) {
            devices.append(VideoDevice{ QString::fromUtf8(device.id()), device.description(),
                                        device.isDefault() });
        }
    }

    return devices;
}

void QtCameraSource::start(const QString &deviceId)
{
    stop();

    const QCameraDevice device = cameraDevice(deviceId);
    if (device.isNull()) {
        qCCritical(lcQtCameraSource) << "Camera not available:" << deviceId;
        Q_EMIT errorOccurred(tr("Camera is not available"));
        return;
    }

    m_camera = std::make_unique<QCamera>(device);
    m_camera->setCameraFormat(bestFormat(device));

    connect(m_camera.get(), &QCamera::activeChanged, this, &CameraSource::activeChanged);
    connect(m_camera.get(), &QCamera::errorOccurred, this,
            [this](QCamera::Error, const QString &message) {
                qCCritical(lcQtCameraSource) << "Camera error:" << message;
                Q_EMIT errorOccurred(message);
            });

    m_captureSession.setCamera(m_camera.get());
    m_camera->start();

    const QString format = formatToString(m_camera->cameraFormat());
    qCInfo(lcQtCameraSource) << "Started" << device.description() << format;
    Q_EMIT formatChanged(format);
}

void QtCameraSource::stop()
{
    if (!m_camera) {
        return;
    }

    m_camera->stop();
    m_captureSession.setCamera(nullptr);
    m_camera.reset();
}

bool QtCameraSource::isActive() const
{
    return m_camera && m_camera->isActive();
}

QCameraDevice QtCameraSource::cameraDevice(const QString &deviceId) const
{
    if (deviceId.isEmpty()) {
        return QMediaDevices::defaultVideoInput();
    }

    const auto videoInputs = m_mediaDevices.videoInputs();
    const auto it = std::ranges::find_if(videoInputs, [&deviceId](const QCameraDevice &device) {
        return QString::fromUtf8(device.id()) == deviceId;
    });

    return it != videoInputs.end() ? *it : QCameraDevice();
}
