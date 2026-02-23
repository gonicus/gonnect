#include <QLoggingCategory>
#include <QRegularExpression>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioInput>

#include "AudioManager.h"
#include "SIPManager.h"
#include "media/AudioPort.h"
#include "GlobalMuteState.h"

Q_LOGGING_CATEGORY(lcAudioManager, "gonnect.sip.audio")

using namespace std::chrono_literals;

AudioManager::AudioManager(QObject *parent) : QObject(parent)
{
#ifdef Q_OS_LINUX
    // PulseAudio handling
    if (!noSyncSystemMute()) {
        m_paMainloop = pa_mainloop_new();
        m_paContext = pa_context_new(pa_mainloop_get_api(m_paMainloop), "GOnnect");

        pa_context_set_state_callback(m_paContext, paContextStateCallback, nullptr);
        pa_context_set_subscribe_callback(m_paContext, paSubscriptionEventCallback, nullptr);

        pa_context_connect(m_paContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

        connect(&m_paMainloopTimer, &QTimer::timeout, this, &AudioManager::paMainloopIterate);
        m_paMainloopTimer.start(100ms);
    }
#endif

    // Use Qt mechanism to get notified for updates and re-initialize on changes
    m_updateDebouncer.setSingleShot(true);
    connect(&m_updateDebouncer, &QTimer::timeout, this, &AudioManager::initialize);

    m_mediaDevices = new QMediaDevices(this);
    connect(m_mediaDevices, &QMediaDevices::audioInputsChanged, this,
            [this]() { m_updateDebouncer.start(500ms); });
    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this,
            [this]() { m_updateDebouncer.start(500ms); });

    connect(this, &AudioManager::captureDeviceIdChanged, this,
            &AudioManager::captureAudioVolumeChanged);

    connect(this, &AudioManager::playbackDeviceIdChanged, this,
            &AudioManager::playbackAudioVolumeChanged);

#ifdef Q_OS_LINUX
    if (!noSyncSystemMute()) {
        connect(this, &AudioManager::isAudioCaptureMutedChanged, this, [this]() {
            if (m_captureAudioPort) {
                paMuteInputByName(m_captureAudioPort->getSystemDeviceID(), m_isAudioCaptureMuted);
                qCInfo(lcAudioManager) << "Sent mute state" << m_isAudioCaptureMuted << "to system";
            } else {
                qCCritical(lcAudioManager) << "Missing capture audio port - cannot set muted flag";
            }
        });
    }
#endif

    connect(&GlobalMuteState::instance(), &GlobalMuteState::isMutedChangedWithTag, this,
            [this](bool value, const QString) { setProperty("isAudioCaptureMuted", value); });
}

AudioManager::~AudioManager()
{
#ifdef Q_OS_LINUX
    if (m_paContext) {
        pa_context_disconnect(m_paContext);
        pa_context_unref(m_paContext);
    }

    if (m_paMainloop) {
        pa_mainloop_free(m_paMainloop);
    }
#endif
}

void AudioManager::initialize()
{
    refreshAudioDevices();
    doProfileElection();

    setCaptureDeviceId(m_captureHash);
    setPlaybackDeviceId(m_playbackHash);
}

#ifdef Q_OS_LINUX
void AudioManager::paMainloopIterate()
{
    if (m_paMainloop) {
        // We won't block for non-queued events (0)
        pa_mainloop_iterate(m_paMainloop, 0, nullptr);
    }
}

void AudioManager::paSubscriptionEventCallback(pa_context *context,
                                               pa_subscription_event_type_t type, uint32_t index,
                                               void *userdata)
{
    Q_UNUSED(userdata)

    if ((type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SOURCE) {
        pa_context_get_source_info_by_index(context, index, paInputMuteStateCallback, nullptr);
    }
}

void AudioManager::paContextStateCallback(pa_context *context, void *userdata)
{
    Q_UNUSED(userdata)

    pa_context_state_t state = pa_context_get_state(context);
    if (state == PA_CONTEXT_READY) {
        pa_context_subscribe(context, PA_SUBSCRIPTION_MASK_SOURCE, nullptr, nullptr);
    }
}

void AudioManager::paMuteInputByName(const QString &name, bool state)
{
    if (m_paContext) {
        pa_context_set_source_mute_by_name(m_paContext, name.toStdString().data(), state, nullptr,
                                           nullptr);
    }
}

void AudioManager::paGetInputMuteState(pa_context *context)
{
    if (context) {
        QString name = AudioManager::instance().m_captureAudioPort->getSystemDeviceID();
        pa_context_get_source_info_by_name(context, name.toStdString().data(),
                                           paInputMuteStateCallback, nullptr);
    }
}

void AudioManager::paInputMuteStateCallback(pa_context *, const pa_source_info *source, int end,
                                            void *)
{
    AudioManager &instance = AudioManager::instance();
    if (!end && instance.m_captureAudioPort->getSystemDeviceID() == QString(source->name)) {
        auto &gms = GlobalMuteState::instance();
        if (source->mute != gms.isMuted()) {
            gms.toggleMute();
        }
    }
}
#endif

void AudioManager::refreshAudioDevices()
{
    qDeleteAll(m_devices);
    m_devices.clear();

    m_devices.push_back(new SIPAudioDevice(tr("Default input"), true, true, this));
    m_devices.push_back(new SIPAudioDevice(tr("Default output"), false, true, this));

    for (auto &d : m_mediaDevices->audioInputs()) {
        m_devices.push_back(new SIPAudioDevice(d.description(), true, false, this));
    }

    for (auto &d : m_mediaDevices->audioOutputs()) {
        m_devices.push_back(new SIPAudioDevice(d.description(), false, false, this));
    }

    Q_EMIT devicesChanged();
}

pj::AudioMedia &AudioManager::getPlaybackDevMedia() const
{
    return *m_playbackAudioPort;
}

pj::AudioMedia &AudioManager::getCaptureDevMedia() const
{
    return *m_captureAudioPort;
}

void AudioManager::setPlaybackDeviceId(const QString &id)
{
    QString newPlaybackHash = id;

    for (auto dev : std::as_const(m_devices)) {
        if (dev->isOutput() && dev->uniqueId() == id) {
            if (m_playbackHash != id) {
                newPlaybackHash = id;
                if (dev->isDefault()) {
                    m_settings.remove(QString("audio%1/playback").arg(m_currentAudioProfile));
                } else {
                    m_settings.setValue(QString("audio%1/playback").arg(m_currentAudioProfile), id);
                }
            }

            qCInfo(lcAudioManager) << "set playback device ID to" << id;
            break;
        }
    }

    if (newPlaybackHash != m_playbackHash) {
        m_playbackHash = newPlaybackHash;
    }

    if (m_playbackHash.isEmpty()) {
        qCInfo(lcAudioManager) << "set playback device to default";
    }

    auto playOutput = getQtAudioOutputForHash(m_playbackHash);
    if (m_playbackAudioPort) {
        m_playbackAudioPort->setAudioDevice(playOutput->device());
    } else {
        m_playbackAudioPort = new AudioPort(playOutput->device());
        if (!m_playbackAudioPort->initialize()) {
            qCWarning(lcAudioManager) << "unable to initialize playback device";
        }

        connect(m_playbackAudioPort, &AudioPort::audioSinkChanged, this,
                &AudioManager::playbackAudioVolumeChanged);
    }

    Q_EMIT playbackDeviceIdChanged();
}

void AudioManager::setCaptureDeviceId(const QString &id)
{
    QString newCaptureHash = id;

    for (auto dev : std::as_const(m_devices)) {
        if (dev->isInput() && dev->uniqueId() == id) {
            if (m_captureHash != id) {
                newCaptureHash = id;
                if (dev->isDefault()) {
                    m_settings.remove(QString("audio%1/capture").arg(m_currentAudioProfile));
                } else {
                    m_settings.setValue(QString("audio%1/capture").arg(m_currentAudioProfile), id);
                }
            }

            qCInfo(lcAudioManager) << "set capture device ID to" << id;
            break;
        }
    }

    if (newCaptureHash != m_captureHash) {
        m_captureHash = newCaptureHash;
    }

    if (m_captureHash.isEmpty()) {
        qCInfo(lcAudioManager) << "set capture device to default";
    }

    auto captureInput = getQtAudioInputForHash(m_captureHash);
    if (m_captureAudioPort) {
        m_captureAudioPort->setAudioDevice(captureInput->device());

    } else {
        m_captureAudioPort = new AudioPort(captureInput->device());
        if (!m_captureAudioPort->initialize()) {
            qCWarning(lcAudioManager) << "unable to initialize capture device";
        }

        connect(m_captureAudioPort, &AudioPort::sourceLevelChanged, this,
                &AudioManager::hasCaptureAudioLevelChanged);
        connect(m_captureAudioPort, &AudioPort::audioSourceChanged, this,
                &AudioManager::captureAudioVolumeChanged);
    }

#ifdef Q_OS_LINUX
    if (!noSyncSystemMute()) {
        // Get the initial mute state of the input device
        paGetInputMuteState(m_paContext);
    }
#endif

    Q_EMIT captureDeviceIdChanged();
}

void AudioManager::setRingDeviceId(const QString &id)
{
    for (auto dev : std::as_const(m_devices)) {
        if (dev->isOutput() && dev->uniqueId() == id) {
            if (m_ringHash != id) {
                m_ringHash = id;
                if (dev->isDefault()) {
                    m_settings.remove(QString("audio%1/ringing").arg(m_currentAudioProfile));
                } else {
                    m_settings.setValue(QString("audio%1/ringing").arg(m_currentAudioProfile), id);
                }
                Q_EMIT ringDeviceIdChanged();
            }

            qCInfo(lcAudioManager) << "set ring device ID to" << id;
            return;
        }
    }

    qCCritical(lcAudioManager) << "unable to set ring device: unknown ID" << id;
}

void AudioManager::setExternalRinger(bool flag)
{
    if (m_externalRinger != flag) {
        m_settings.setValue(QString("audio%1/preferExternalRinger").arg(m_currentAudioProfile),
                            flag);
        qCInfo(lcAudioManager) << "set external ringer to" << flag;
        Q_EMIT externalRingerChanged();
    }
}

bool AudioManager::isDeviceAvailable(const QString &hash)
{
    // Default hash is empty
    if (hash.isEmpty()) {
        return true;
    }

    for (auto dev : std::as_const(m_devices)) {
        if (dev->uniqueId() == hash) {
            return true;
        }
    }

    return false;
}

void AudioManager::doProfileElection()
{
    bool found = false;
    unsigned pc = 0;

    static QRegularExpression isAudioProfile = QRegularExpression("^audio[0-9]+$");
    QStringList groups = m_settings.childGroups();
    QString playbackHash;
    QString captureHash;
    QString ringHash;

    for (auto &group : std::as_const(groups)) {
        if (isAudioProfile.match(group).hasMatch()) {
            playbackHash = m_settings.value(QString("%1/playback").arg(group)).toString();
            captureHash = m_settings.value(QString("%1/capture").arg(group)).toString();
            ringHash = m_settings.value(QString("%1/ringing").arg(group)).toString();
            m_externalRinger =
                    m_settings.value(QString("%1/preferExternalRinger").arg(group), false).toBool();

            // Check if all audio hashes are available for this profile
            if (isDeviceAvailable(playbackHash) && isDeviceAvailable(captureHash)
                && isDeviceAvailable(ringHash)) {
                m_playbackHash = playbackHash;
                Q_EMIT playbackDeviceIdChanged();

                m_captureHash = captureHash;
                Q_EMIT captureDeviceIdChanged();

                m_ringHash = ringHash;
                Q_EMIT ringDeviceIdChanged();

                m_currentAudioProfile = pc;
                Q_EMIT currentProfileChanged();
                found = true;
                break;
            }

            pc++;
        }
    }

    if (found) {
        Q_EMIT matchingAudioProfile();
    } else {
        m_currentAudioProfile = pc;
        if (SIPManager::instance().isConfigured()) {
            Q_EMIT noMatchingAudioProfile();
        }

        qCWarning(lcAudioManager) << "no matching audio profile found - using defaults";
    }

    // Load hashes for default devices if nothing is set
    if (m_devices.count() >= 2) {
        auto defaultCaptureDevice = m_devices.first();
        auto defaultPlaybackDevice = m_devices.value(1);

        if (m_playbackHash.isEmpty()) {
            m_playbackHash = defaultPlaybackDevice->uniqueId();
            Q_EMIT playbackDeviceIdChanged();
        }

        if (m_captureHash.isEmpty()) {
            m_captureHash = defaultCaptureDevice->uniqueId();
            Q_EMIT captureDeviceIdChanged();
        }

        if (m_ringHash.isEmpty()) {
            m_ringHash = defaultPlaybackDevice->uniqueId();
            Q_EMIT ringDeviceIdChanged();
        }
    }
}

QAudioOutput *AudioManager::getQtAudioOutputForHash(const QString &id)
{
    QAudioOutput *dev = nullptr;
    bool isDefaultDevice = false;

    if (m_devices.count() >= 2) {
        if (id == m_devices.value(1)->uniqueId()) {
            isDefaultDevice = true;
        }
    }

    if (!isDefaultDevice) {
        const QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();
        for (const QAudioDevice &device : audioDevices) {
            auto calculatedHash = SIPAudioDevice::makeHash(device.description(), false);
            if (calculatedHash == id) {
                dev = new QAudioOutput(device, this);
                break;
            }
        }
    }

    if (!dev) {
        if (!isDefaultDevice) {
            qCWarning(lcAudioManager) << "audio device not found - using default";
        }

        dev = new QAudioOutput(QMediaDevices::defaultAudioOutput(), this);
    }

    return dev;
}

QAudioInput *AudioManager::getQtAudioInputForHash(const QString &id)
{
    QAudioInput *dev = nullptr;
    bool isDefaultDevice = false;

    if (m_devices.count() >= 2) {
        if (id == m_devices.first()->uniqueId()) {
            isDefaultDevice = true;
        }
    }

    if (!isDefaultDevice) {
        const QList<QAudioDevice> audioDevices = QMediaDevices::audioInputs();
        for (const QAudioDevice &device : std::as_const(audioDevices)) {
            auto calculatedHash = SIPAudioDevice::makeHash(device.description(), true);
            if (calculatedHash == id) {
                dev = new QAudioInput(device, this);
                break;
            }
        }
    }

    if (!dev) {
        if (!isDefaultDevice) {
            qCWarning(lcAudioManager) << "audio device not found - using default";
        }

        dev = new QAudioInput(QMediaDevices::defaultAudioInput(), this);
    }

    return dev;
}

bool AudioManager::hasCaptureAudioLevel() const
{
    if (m_captureAudioPort) {
        return m_captureAudioPort->sourceLevel() > 0.4;
    }
    return 0.0;
}

qreal AudioManager::captureAudioVolume() const
{
    if (m_captureAudioPort) {
        const auto source = m_captureAudioPort->audioSource();
        if (source) {
            return source->volume();
        }
    }

    return 0.0;
}

void AudioManager::setCaptureAudioVolume(qreal volume)
{
    Q_ASSERT(0.0 <= volume && volume <= 1.0);

    if (m_captureAudioPort) {
        auto source = m_captureAudioPort->audioSource();
        if (source && source->volume() != volume) {
            source->setVolume(volume);
            Q_EMIT captureAudioVolumeChanged();
        }
    }
}

qreal AudioManager::playbackAudioVolume() const
{
    if (m_playbackAudioPort) {
        const auto sink = m_playbackAudioPort->audioSink();
        if (sink) {
            return sink->volume();
        }
    }
    return 0.0;
}

void AudioManager::setPlaybackAudioVolume(qreal volume)
{
    Q_ASSERT(0.0 <= volume && volume <= 1.0);

    if (m_playbackAudioPort) {
        auto sink = m_playbackAudioPort->audioSink();

        if (sink && sink->volume() != volume) {
            sink->setVolume(volume);
            Q_EMIT playbackAudioVolumeChanged();
        }
    }
}
