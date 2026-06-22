#include <QLoggingCategory>
#include "LinuxPlatformSession.h"
#include "GlobalMuteState.h"

Q_LOGGING_CATEGORY(lcPlatformSession, "gonnect.platform.session")

using namespace std::chrono_literals;

PlatformSession &PlatformSession::instance()
{
    static PlatformSession *_instance = nullptr;
    if (!_instance) {
        _instance = new LinuxPlatformSession;
    }
    return *_instance;
}

LinuxPlatformSession::LinuxPlatformSession(QObject *parent) : PlatformSession(parent) { }

LinuxPlatformSession::~LinuxPlatformSession()
{
    LinuxPlatformSession::stop();
}

void LinuxPlatformSession::start(bool enableMuteSync)
{
    // PulseAudio
    if (enableMuteSync && !m_paMainloop) {
        m_paMainloop = pa_mainloop_new();
        m_paContext = pa_context_new(pa_mainloop_get_api(m_paMainloop), "GOnnect");

        pa_context_set_state_callback(m_paContext, contextStateCallback, nullptr);
        pa_context_set_subscribe_callback(m_paContext, subscriptionEventCallback, nullptr);

        pa_context_connect(m_paContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

        connect(&m_mainloopTimer, &QTimer::timeout, this, &LinuxPlatformSession::mainloopIterate);
        m_mainloopTimer.start(100ms);
    }

    // Pipewire
    if (!m_pwContext) {
        pw_init(nullptr, nullptr);

        m_pwMainLoop = pw_main_loop_new(nullptr);
        m_pwLoop = pw_main_loop_get_loop(m_pwMainLoop);
        m_pwContext = pw_context_new(m_pwLoop, nullptr, 0);
        m_pwCore = pw_context_connect(m_pwContext, nullptr, 0);

        if (m_pwCore) {
            m_pwRegistry = pw_core_get_registry(m_pwCore, PW_VERSION_REGISTRY, 0);

            static const pw_registry_events registryEvents = {
                .version = PW_VERSION_REGISTRY_EVENTS,
                .global = pwRegistryGlobal,
                .global_remove = pwRegistryGlobalRemove,
            };
            pw_registry_add_listener(m_pwRegistry, &m_pwRegistryListener, &registryEvents, nullptr);
        }
    }
}

void LinuxPlatformSession::stop()
{
    // Pipewire
    if (m_pwRegistry) {
        spa_hook_remove(&m_pwRegistryListener);
        pw_proxy_destroy((struct pw_proxy *)m_pwRegistry);
        m_pwRegistry = nullptr;
    }
    if (m_pwCore) {
        pw_core_disconnect(m_pwCore);
        m_pwCore = nullptr;
    }
    if (m_pwContext) {
        pw_context_destroy(m_pwContext);
        m_pwContext = nullptr;
    }
    if (m_pwMainLoop) {
        pw_main_loop_destroy(m_pwMainLoop);
        m_pwMainLoop = nullptr;
    }
    m_pwLoop = nullptr;
    m_screenCastNodeIds.clear();

    // PulseAudio
    m_mainloopTimer.stop();

    if (m_paContext) {
        pa_context_disconnect(m_paContext);
        pa_context_unref(m_paContext);
        m_paContext = nullptr;
    }

    if (m_paMainloop) {
        pa_mainloop_free(m_paMainloop);
        m_paMainloop = nullptr;
    }
}

void LinuxPlatformSession::pwRegistryGlobal(void *data, uint32_t id, uint32_t permissions,
                                            const char *type, uint32_t version,
                                            const struct spa_dict *props)
{
    Q_UNUSED(data)
    Q_UNUSED(permissions)
    Q_UNUSED(version)

    auto &self = static_cast<LinuxPlatformSession &>(PlatformSession::instance());

    if (props && strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
        const char *mediaClass = spa_dict_lookup(props, "media.class");
        if (mediaClass && strcmp(mediaClass, "Stream/Output/Video") == 0) {
            self.m_screenCastNodeIds.insert(id);
            if (self.m_screenCastNodeIds.size() == 1) {
                self.setScreenShareActive(true);
            }
        }
    }
}

void LinuxPlatformSession::pwRegistryGlobalRemove(void *data, uint32_t id)
{
    Q_UNUSED(data)

    auto &self = static_cast<LinuxPlatformSession &>(PlatformSession::instance());

    if (self.m_screenCastNodeIds.remove(id) && self.m_screenCastNodeIds.isEmpty()) {
        self.setScreenShareActive(false);
    }
}

void LinuxPlatformSession::setScreenShareActive(bool active)
{
    if (m_screenShareActive != active) {
        m_screenShareActive = active;
        Q_EMIT screenShareActiveChanged(active);
    }
}

void LinuxPlatformSession::mainloopIterate()
{
    if (m_paMainloop) {
        pa_mainloop_iterate(m_paMainloop, 0, nullptr);
    }
    if (m_pwLoop) {
        pw_loop_iterate(m_pwLoop, 0);
    }
}

void LinuxPlatformSession::subscriptionEventCallback(pa_context *context,
                                                     pa_subscription_event_type_t type,
                                                     uint32_t index, void *userdata)
{
    Q_UNUSED(userdata)

    if ((type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SOURCE) {
        pa_context_get_source_info_by_index(context, index, inputMuteStateCallback, nullptr);
    }
}

void LinuxPlatformSession::contextStateCallback(pa_context *context, void *userdata)
{
    Q_UNUSED(userdata)

    pa_context_state_t state = pa_context_get_state(context);
    if (state == PA_CONTEXT_READY) {
        pa_context_subscribe(context, PA_SUBSCRIPTION_MASK_SOURCE, nullptr, nullptr);
    }
}

void LinuxPlatformSession::muteInputByName(const QString &name, bool state)
{
    if (m_paContext) {
        pa_context_set_source_mute_by_name(m_paContext, name.toStdString().data(), state, nullptr,
                                           nullptr);
    }
}

void LinuxPlatformSession::getInputMuteState()
{
    if (m_paContext && !m_captureDeviceId.isEmpty()) {
        pa_context_get_source_info_by_name(m_paContext, m_captureDeviceId.toStdString().data(),
                                           inputMuteStateCallback, nullptr);
    }
}

void LinuxPlatformSession::inputMuteStateCallback(pa_context *, const pa_source_info *source,
                                                  int end, void *)
{
    auto &self = static_cast<LinuxPlatformSession &>(PlatformSession::instance());

    if (!end && source && self.m_captureDeviceId == QString(source->name)) {
        if (self.m_callbackSuppress > 0) {
            self.m_callbackSuppress--;
            return;
        }

        if (source->mute != GlobalMuteState::instance().isMuted()) {
            Q_EMIT self.systemMuteChanged(source->mute);
        }
    }
}

void LinuxPlatformSession::setCaptureDeviceId(const QString &systemDeviceId)
{
    m_captureDeviceId = systemDeviceId;

    if (!m_captureDeviceId.isEmpty()) {
        getInputMuteState();
    }
}

void LinuxPlatformSession::syncSystemMute(bool muted)
{
    if (!m_captureDeviceId.isEmpty()) {
        m_callbackSuppress++;
        muteInputByName(m_captureDeviceId, muted);
        qCInfo(lcPlatformSession) << "Sent mute state" << muted << "to system";
    }
}
