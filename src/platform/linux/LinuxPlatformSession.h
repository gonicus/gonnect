#pragma once

#include <pulse/pulseaudio.h>
#include <QTimer>
#include "PlatformSession.h"

class LinuxPlatformSession : public PlatformSession
{
    Q_OBJECT
    Q_DISABLE_COPY(LinuxPlatformSession)

public:
    explicit LinuxPlatformSession(QObject *parent = nullptr);
    ~LinuxPlatformSession() override;

    void start() override;
    void stop() override;
    void setCaptureDeviceId(const QString &systemDeviceId) override;
    void syncSystemMute(bool muted) override;

private Q_SLOTS:
    void mainloopIterate();

private:
    static void subscriptionEventCallback(pa_context *context, pa_subscription_event_type_t type,
                                          uint32_t index, void *userdata);
    static void contextStateCallback(pa_context *context, void *userdata);
    void muteInputByName(const QString &name, bool state);
    void getInputMuteState();
    static void inputMuteStateCallback(pa_context *context, const pa_source_info *source, int end,
                                       void *userdata);

    pa_mainloop *m_paMainloop = nullptr;
    pa_context *m_paContext = nullptr;
    QTimer m_mainloopTimer;
    int m_callbackSuppress = 0;
    QString m_captureDeviceId;
};
