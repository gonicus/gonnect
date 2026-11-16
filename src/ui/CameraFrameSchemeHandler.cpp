#include "CameraFrameSchemeHandler.h"
#include "GlobalInfo.h"
#include "VideoManager.h"

#include <QBuffer>
#include <QImage>
#include <QLoggingCategory>
#include <QQuickWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>
#include <QtEndian>

#include <chrono>
#include <utility>

Q_LOGGING_CATEGORY(lcCameraFrameSchemeHandler, "gonnect.media.camera.scheme")

namespace {
constexpr auto IdleTimeout = std::chrono::seconds(1);

enum class FrameFormat : quint32 { I420 = 0, RGBX = 1 };

void appendUint32(QByteArray &data, quint32 value)
{
    const quint32 littleEndian = qToLittleEndian(value);
    data.append(reinterpret_cast<const char *>(&littleEndian), sizeof(littleEndian));
}

void appendHeader(QByteArray &data, FrameFormat format, int width, int height)
{
    appendUint32(data, static_cast<quint32>(format));
    appendUint32(data, static_cast<quint32>(width));
    appendUint32(data, static_cast<quint32>(height));
}

void appendRows(QByteArray &data, const uchar *bits, qsizetype bytesPerLine, int rowLength,
                int rowCount)
{
    for (int y = 0; y < rowCount; ++y) {
        data.append(reinterpret_cast<const char *>(bits + y * bytesPerLine), rowLength);
    }
}

QByteArray serializeI420(QVideoFrame frame)
{
    if (!frame.map(QVideoFrame::ReadOnly)) {
        return {};
    }

    const int width = frame.width();
    const int height = frame.height();
    const int chromaWidth = (width + 1) / 2;
    const int chromaHeight = (height + 1) / 2;

    QByteArray data;
    data.reserve(12 + width * height + 2 * chromaWidth * chromaHeight);
    appendHeader(data, FrameFormat::I420, width, height);
    appendRows(data, frame.bits(0), frame.bytesPerLine(0), width, height);
    appendRows(data, frame.bits(1), frame.bytesPerLine(1), chromaWidth, chromaHeight);
    appendRows(data, frame.bits(2), frame.bytesPerLine(2), chromaWidth, chromaHeight);

    frame.unmap();
    return data;
}

QByteArray serializeRgbx(const QVideoFrame &frame)
{
    const QImage image = frame.toImage().convertToFormat(QImage::Format_RGBX8888);
    if (image.isNull()) {
        return {};
    }

    QByteArray data;
    data.reserve(12 + image.width() * image.height() * 4);
    appendHeader(data, FrameFormat::RGBX, image.width(), image.height());
    appendRows(data, image.constBits(), image.bytesPerLine(), image.width() * 4, image.height());
    return data;
}

QByteArray serialize(const QVideoFrame &frame)
{
    if (frame.pixelFormat() == QVideoFrameFormat::Format_YUV420P) {
        return serializeI420(frame);
    }

    return serializeRgbx(frame);
}

} // namespace

void CameraFrameSchemeHandler::registerScheme()
{
    QWebEngineUrlScheme scheme(Scheme);
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::CorsEnabled
                    | QWebEngineUrlScheme::FetchApiAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
}

void CameraFrameSchemeHandler::install(QQuickWebEngineProfile *profile)
{
    if (!profile) {
        qCWarning(lcCameraFrameSchemeHandler) << "Cannot install on null profile";
        return;
    }

    if (profile->urlSchemeHandler(Scheme) != this) {
        profile->installUrlSchemeHandler(Scheme, this);
    }
}

void CameraFrameSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job)
{
    if (!isJitsiRequest(job) || job->requestUrl().path() != QLatin1String("frame")) {
        qCWarning(lcCameraFrameSchemeHandler)
                << "Denying camera frame request from" << job->initiator() << job->requestUrl();
        job->fail(QWebEngineUrlRequestJob::RequestDenied);
        return;
    }

    if (VideoManager::instance().isCameraAccessDenied()) {
        job->fail(QWebEngineUrlRequestJob::RequestDenied);
        return;
    }

    m_pendingJobs.append(job);
    m_idleTimer.start();
    setActive(true);
}

CameraFrameSchemeHandler::CameraFrameSchemeHandler(QObject *parent)
    : QWebEngineUrlSchemeHandler{ parent }
{
    m_framePump.setSink(&m_videoSink);
    connect(&m_videoSink, &QVideoSink::videoFrameChanged, this,
            &CameraFrameSchemeHandler::answerPendingJobs);

    m_idleTimer.setSingleShot(true);
    m_idleTimer.setInterval(IdleTimeout);
    m_idleTimer.callOnTimeout(this, &CameraFrameSchemeHandler::stopWhenIdle);
}

bool CameraFrameSchemeHandler::isJitsiRequest(const QWebEngineUrlRequestJob *job) const
{
    const QUrl jitsiUrl(GlobalInfo::instance().jitsiUrl());
    const QUrl initiator = job->initiator();

    return !jitsiUrl.host().isEmpty() && initiator.scheme() == jitsiUrl.scheme()
            && initiator.host() == jitsiUrl.host() && initiator.port() == jitsiUrl.port();
}

void CameraFrameSchemeHandler::answerPendingJobs(const QVideoFrame &frame)
{
    m_pendingJobs.removeAll(nullptr);

    if (m_pendingJobs.isEmpty()) {
        return;
    }

    const QByteArray data = serialize(frame);
    if (data.isEmpty()) {
        qCWarning(lcCameraFrameSchemeHandler) << "Cannot serialize frame" << frame.pixelFormat();
        return;
    }

    const auto jobs = std::exchange(m_pendingJobs, {});
    for (const auto &job : jobs) {
        if (!job) {
            continue;
        }

        job->setAdditionalResponseHeaders(
                { { "Access-Control-Allow-Origin", job->initiator().toString().toUtf8() },
                  { "Cache-Control", "no-store" } });

        // Do not touch buffer after reply() because it is read in another thread
        auto *buffer = new QBuffer(job);
        buffer->setData(data);
        job->reply("application/octet-stream", buffer);
    }
}

void CameraFrameSchemeHandler::stopWhenIdle()
{
    m_pendingJobs.removeAll(nullptr);
    if (!m_pendingJobs.isEmpty()) {
        m_idleTimer.start();
        return;
    }

    setActive(false);
}

void CameraFrameSchemeHandler::setActive(bool active)
{
    if (m_isActive == active) {
        return;
    }

    m_isActive = active;

    if (m_isActive) {
        qCInfo(lcCameraFrameSchemeHandler) << "Camera frames requested - starting camera";
        VideoManager::instance().addConsumer(&m_framePump);
    } else {
        qCInfo(lcCameraFrameSchemeHandler) << "No more camera frame requests - stopping camera";
        VideoManager::instance().removeConsumer(&m_framePump);
    }
}
