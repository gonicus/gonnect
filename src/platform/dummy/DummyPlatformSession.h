#pragma once

#include <QObject>
#include "PlatformSession.h"

class DummyPlatformSession : public PlatformSession
{
    Q_OBJECT
    Q_DISABLE_COPY(DummyPlatformSession)

public:
    explicit DummyPlatformSession() : PlatformSession() { }
    ~DummyPlatformSession() override = default;

    void start() override { }
    void stop() override { }
    void setCaptureDeviceId(const QString &) override { }
    void syncSystemMute(bool) override { }
};
