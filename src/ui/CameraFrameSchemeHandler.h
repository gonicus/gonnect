#pragma once

#include "FramePump.h"

#include <QList>
#include <QPointer>
#include <QQmlEngine>
#include <QTimer>
#include <QVideoSink>
#include <QWebEngineUrlSchemeHandler>

class QQuickWebEngineProfile;
class QWebEngineUrlRequestJob;

/// Serves camera frames at "gonnect-camera:frame. Each request is responded with the next new
/// frame.
class CameraFrameSchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    static constexpr char Scheme[] = "gonnect-camera";

    static CameraFrameSchemeHandler &instance()
    {
        static CameraFrameSchemeHandler _instance;
        return _instance;
    }

    /// Must be called before QtWebEngineQuick::initialize()
    static void registerScheme();

    Q_INVOKABLE void install(QQuickWebEngineProfile *profile);

    void requestStarted(QWebEngineUrlRequestJob *job) override;

private:
    explicit CameraFrameSchemeHandler(QObject *parent = nullptr);

    bool isJitsiRequest(const QWebEngineUrlRequestJob *job) const;
    void answerPendingJobs(const QVideoFrame &frame);
    void stopWhenIdle();
    void setActive(bool active);

    FramePump m_framePump;
    QVideoSink m_videoSink;
    QList<QPointer<QWebEngineUrlRequestJob>> m_pendingJobs;
    QTimer m_idleTimer;
    bool m_isActive = false;
};

class CameraFrameSchemeHandlerWrapper
{
    Q_GADGET
    QML_FOREIGN(CameraFrameSchemeHandler)
    QML_NAMED_ELEMENT(CameraFrameSchemeHandler)
    QML_SINGLETON

public:
    static CameraFrameSchemeHandler *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&CameraFrameSchemeHandler::instance(),
                                       QQmlEngine::CppOwnership);
        return &CameraFrameSchemeHandler::instance();
    }

private:
    CameraFrameSchemeHandlerWrapper() = default;
};
