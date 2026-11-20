#include "CameraFrameSchemeHandler.h"
#include "CameraFrameSerializer.h"
#include "GlobalInfo.h"
#include "OriginHelper.h"
#include "VideoManager.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QQuickWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>

#include <chrono>
#include <utility>

Q_LOGGING_CATEGORY(lcCameraFrameSchemeHandler, "gonnect.media.camera.scheme")

namespace {
constexpr auto IdleTimeout = std::chrono::seconds(1);

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

    if (!VideoManager::instance().isCameraAvailable()) {
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

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() { setActive(false); });

    // Waiting requests would never be answered after last camera gone
    auto &videoManager = VideoManager::instance();
    connect(&videoManager, &VideoManager::devicesChanged, this,
            &CameraFrameSchemeHandler::failPendingJobsIfUnavailable);
    connect(&videoManager, &VideoManager::cameraAccessStateChanged, this,
            &CameraFrameSchemeHandler::failPendingJobsIfUnavailable);
    connect(&videoManager, &VideoManager::cameraErrorOccurred, this,
            &CameraFrameSchemeHandler::failPendingJobs);
}

bool CameraFrameSchemeHandler::isJitsiRequest(const QWebEngineUrlRequestJob *job) const
{
    return OriginHelper::isSameOrigin(job->initiator(), QUrl(GlobalInfo::instance().jitsiUrl()));
}

void CameraFrameSchemeHandler::answerPendingJobs(const QVideoFrame &frame)
{
    m_pendingJobs.removeAll(nullptr);

    if (m_pendingJobs.isEmpty()) {
        return;
    }

    const QByteArray data = CameraFrameSerializer::serialize(frame);
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

void CameraFrameSchemeHandler::failPendingJobs()
{
    const auto jobs = std::exchange(m_pendingJobs, {});
    for (const auto &job : jobs) {
        if (job) {
            job->fail(QWebEngineUrlRequestJob::RequestDenied);
        }
    }
}

void CameraFrameSchemeHandler::failPendingJobsIfUnavailable()
{
    if (!VideoManager::instance().isCameraAvailable()) {
        failPendingJobs();
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
