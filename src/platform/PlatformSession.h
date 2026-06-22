#pragma once

#include <QObject>

class PlatformSession : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PlatformSession)

public:
    Q_REQUIRED_RESULT static PlatformSession &instance();

    explicit PlatformSession(QObject *parent = nullptr) : QObject(parent) { }
    ~PlatformSession() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setCaptureDeviceId(const QString &systemDeviceId) = 0;
    virtual void syncSystemMute(bool muted) = 0;
    virtual bool isScreenShareActive() const = 0;

Q_SIGNALS:
    void systemMuteChanged(bool muted);
    void screenShareActiveChanged(bool active);
};
