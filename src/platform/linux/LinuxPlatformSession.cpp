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
    stop();
}

void LinuxPlatformSession::start()
{
    if (m_paMainloop) {
        return;
    }

    m_paMainloop = pa_mainloop_new();
    m_paContext = pa_context_new(pa_mainloop_get_api(m_paMainloop), "GOnnect");

    pa_context_set_state_callback(m_paContext, contextStateCallback, nullptr);
    pa_context_set_subscribe_callback(m_paContext, subscriptionEventCallback, nullptr);

    pa_context_connect(m_paContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    connect(&m_mainloopTimer, &QTimer::timeout, this, &LinuxPlatformSession::mainloopIterate);
    m_mainloopTimer.start(100ms);
}

void LinuxPlatformSession::stop()
{
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

void LinuxPlatformSession::mainloopIterate()
{
    if (m_paMainloop) {
        pa_mainloop_iterate(m_paMainloop, 0, nullptr);
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
