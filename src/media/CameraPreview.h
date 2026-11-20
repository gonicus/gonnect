#pragma once

#include "FramePump.h"

#include <QObject>
#include <QPointer>
#include <QVideoSink>
#include <qqmlintegration.h>

/// Feeds camera image into VideoOutput; camera runs while preview active.
class CameraPreview : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
            QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged FINAL)
    Q_PROPERTY(bool isActive READ isActive WRITE setIsActive NOTIFY isActiveChanged FINAL)

public:
    explicit CameraPreview(QObject *parent = nullptr);
    ~CameraPreview() override;

    QVideoSink *videoSink() const { return m_videoSink; }
    void setVideoSink(QVideoSink *sink);

    bool isActive() const { return m_isActive; }
    void setIsActive(bool value);

Q_SIGNALS:
    void videoSinkChanged();
    void isActiveChanged();

private:
    QPointer<QVideoSink> m_videoSink;
    FramePump m_framePump;
    bool m_isActive = false;
};
