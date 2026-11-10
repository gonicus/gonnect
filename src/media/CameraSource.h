#pragma once

#include "VideoDevice.h"

#include <QObject>
#include <QMutex>
#include <QVideoFrame>

#include <functional>

class CameraSource : public QObject
{
    Q_OBJECT

public:
    using FrameHandler = std::function<void(const QVideoFrame &)>;

    explicit CameraSource(QObject *parent = nullptr);

    virtual QList<VideoDevice> devices() const = 0;
    virtual void start(const QString &devicId) = 0;
    virtual void stop() = 0;
    virtual bool isActive() const = 0;

    void setFrameHandler(FrameHandler handler);

Q_SIGNALS:
    void devicesChanged();
    void activeChanged();
    void formatChanged(const QString &description);
    void errorOccurred(const QString &message);

protected:
    void deliverFrame(const QVideoFrame &frame);

private:
    QMutex m_frameHandlerMutex;
    FrameHandler m_frameHandler;
};
