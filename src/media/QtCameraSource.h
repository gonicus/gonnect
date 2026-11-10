#pragma once

#include "CameraSource.h"

#include <QCamera>
#include <QCameraDevice>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QVideoSink>

class QtCameraSource : public CameraSource
{
    Q_OBJECT

public:
    explicit QtCameraSource(QObject *parent = nullptr);
    ~QtCameraSource() override;

    QList<VideoDevice> devices() const override;
    void start(const QString &deviceId) override;
    void stop() override;
    bool isActive() const override;

private:
    QCameraDevice cameraDevice(const QString &deviceId) const;

    QMediaDevices m_mediaDevices;
    QVideoSink m_videoSink;
    QMediaCaptureSession m_captureSession;
    std::unique_ptr<QCamera> m_camera;
};
