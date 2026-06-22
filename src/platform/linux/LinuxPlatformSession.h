#pragma once

#include <pulse/pulseaudio.h>
#include <pipewire/pipewire.h>
#include <QSet>
#include <QTimer>
#include "../PlatformSession.h"

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
    bool isScreenShareActive() const override { return m_screenShareActive; }

private Q_SLOTS:
    void mainloopIterate();

private:
    // PulseAudio
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

    // Pipewire
    static void pwRegistryGlobal(void *data, uint32_t id, uint32_t permissions, const char *type,
                                 uint32_t version, const struct spa_dict *props);
    static void pwRegistryGlobalRemove(void *data, uint32_t id);
    void setScreenShareActive(bool active);

    struct pw_main_loop *m_pwMainLoop = nullptr;
    struct pw_loop *m_pwLoop = nullptr;
    struct pw_context *m_pwContext = nullptr;
    struct pw_core *m_pwCore = nullptr;
    struct pw_registry *m_pwRegistry = nullptr;
    struct spa_hook m_pwRegistryListener;
    QSet<uint32_t> m_screenCastNodeIds;
    bool m_screenShareActive = false;
};
