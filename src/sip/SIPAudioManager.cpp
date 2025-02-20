#include <QLoggingCategory>
#include <QRegularExpression>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioInput>

#include "SIPAudioManager.h"
#include "SIPManager.h"
#include "media/AudioPort.h"

#include "HeadsetDeviceProxy.h"
#include "USBDevices.h"

Q_LOGGING_CATEGORY(lcSIPAudioManager, "gonnect.sip.audio")

using namespace std::chrono_literals;

SIPAudioManager::SIPAudioManager(QObject *parent) : QObject(parent)
{
    m_settings = std::make_unique<AppSettings>();

    // Use Qt mechanism to get notified for updates and re-initialize on changes
    m_updateDebouncer.setSingleShot(true);
    connect(&m_updateDebouncer, &QTimer::timeout, this, &SIPAudioManager::initialize);

    m_mediaDevices = new QMediaDevices(this);
    connect(m_mediaDevices, &QMediaDevices::audioInputsChanged, this,
            [this]() { m_updateDebouncer.start(500ms); });
    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this,
            [this]() { m_updateDebouncer.start(500ms); });

    connect(this, &SIPAudioManager::captureDeviceIdChanged, this,
            &SIPAudioManager::captureAudioVolumeChanged);

    connect(this, &SIPAudioManager::playbackDeviceIdChanged, this,
            &SIPAudioManager::playbackAudioVolumeChanged);

    connect(this, &SIPAudioManager::isAudioCaptureMutedChanged, this, [this]() {
        if (m_captureAudioPort) {
            m_captureAudioPort->setMuted(m_isAudioCaptureMuted);
        } else {
            qCCritical(lcSIPAudioManager) << "Missing capture audio port - cannot set muted flag";
        }

        // Headset device mute handling
        auto &hds = USBDevices::instance();
        auto dev = hds.getHeadsetDeviceProxy();
        dev->setMute(m_isAudioCaptureMuted);
    });

    // Headset device mute handling
    auto &hds = USBDevices::instance();
    auto dev = hds.getHeadsetDeviceProxy();
    connect(dev, &HeadsetDeviceProxy::mute, this,
            [this, dev]() { setProperty("isAudioCaptureMuted", dev->getMute()); });
}

void SIPAudioManager::initialize()
{
    refreshAudioDevices();
    doProfileElection();

    setCaptureDeviceId(m_captureHash);
    setPlaybackDeviceId(m_playbackHash);
}

void SIPAudioManager::refreshAudioDevices()
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

    emit devicesChanged();
}

pj::AudioMedia &SIPAudioManager::getPlaybackDevMedia() const
{
    return *m_playbackAudioPort;
}

pj::AudioMedia &SIPAudioManager::getCaptureDevMedia() const
{
    return *m_captureAudioPort;
}

void SIPAudioManager::setPlaybackDeviceId(const QString &id)
{
    QString newPlaybackHash = id;

    for (auto dev : std::as_const(m_devices)) {
        if (dev->isOutput() && dev->uniqueId() == id) {
            if (m_playbackHash != id) {
                newPlaybackHash = id;
                if (dev->isDefault()) {
                    m_settings->remove(QString("audio%1/playback").arg(m_currentAudioProfile));
                } else {
                    m_settings->setValue(QString("audio%1/playback").arg(m_currentAudioProfile),
                                         id);
                }
            }

            qCInfo(lcSIPAudioManager) << "set playback device ID to" << id;
            break;
        }
    }

    if (newPlaybackHash != m_playbackHash) {
        m_playbackHash = newPlaybackHash;
    }

    if (m_playbackHash.isEmpty()) {
        qCInfo(lcSIPAudioManager) << "set playback device to default";
    }

    auto playOutput = getQtAudioOutputForHash(m_playbackHash);
    if (m_playbackAudioPort) {
        m_playbackAudioPort->setAudioDevice(playOutput->device());
    } else {
        m_playbackAudioPort = new AudioPort(playOutput->device());
        if (!m_playbackAudioPort->initialize()) {
            qCWarning(lcSIPAudioManager) << "unable to initialize playback device";
        }

        connect(m_playbackAudioPort, &AudioPort::audioSinkChanged, this,
                &SIPAudioManager::playbackAudioVolumeChanged);
    }

    emit playbackDeviceIdChanged();
}

void SIPAudioManager::setCaptureDeviceId(const QString &id)
{
    QString newCaptureHash = id;

    for (auto dev : std::as_const(m_devices)) {
        if (dev->isInput() && dev->uniqueId() == id) {
            if (m_captureHash != id) {
                newCaptureHash = id;
                if (dev->isDefault()) {
                    m_settings->remove(QString("audio%1/capture").arg(m_currentAudioProfile));
                } else {
                    m_settings->setValue(QString("audio%1/capture").arg(m_currentAudioProfile), id);
                }
            }

            qCInfo(lcSIPAudioManager) << "set capture device ID to" << id;
            break;
        }
    }

    if (newCaptureHash != m_captureHash) {
        m_captureHash = newCaptureHash;
    }

    if (m_captureHash.isEmpty()) {
        qCInfo(lcSIPAudioManager) << "set capture device to default";
    }

    auto captureInput = getQtAudioInputForHash(m_captureHash);
    if (m_captureAudioPort) {
        m_captureAudioPort->setAudioDevice(captureInput->device());

    } else {
        m_captureAudioPort = new AudioPort(captureInput->device());
        if (!m_captureAudioPort->initialize()) {
            qCWarning(lcSIPAudioManager) << "unable to initialize capture device";
        }

        connect(m_captureAudioPort, &AudioPort::sourceLevelChanged, this,
                &SIPAudioManager::hasCaptureAudioLevelChanged);
        connect(m_captureAudioPort, &AudioPort::audioSourceChanged, this,
                &SIPAudioManager::captureAudioVolumeChanged);
    }

    emit captureDeviceIdChanged();
}

void SIPAudioManager::setRingDeviceId(const QString &id)
{
    for (auto dev : std::as_const(m_devices)) {
        if (dev->isOutput() && dev->uniqueId() == id) {
            if (m_ringHash != id) {
                m_ringHash = id;
                if (dev->isDefault()) {
                    m_settings->remove(QString("audio%1/ringing").arg(m_currentAudioProfile));
                } else {
                    m_settings->setValue(QString("audio%1/ringing").arg(m_currentAudioProfile), id);
                }
                emit ringDeviceIdChanged();
            }

            qCInfo(lcSIPAudioManager) << "set ring device ID to" << id;
            return;
        }
    }

    qCCritical(lcSIPAudioManager) << "unable to set ring device: unknown ID" << id;
}

void SIPAudioManager::setExternalRinger(bool flag)
{
    if (m_externalRinger != flag) {
        m_settings->setValue(QString("audio%1/preferExternalRinger").arg(m_currentAudioProfile),
                             flag);
        qCInfo(lcSIPAudioManager) << "set external ringer to" << flag;
        emit externalRingerChanged();
    }
}

bool SIPAudioManager::isDeviceAvailable(const QString &hash)
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

void SIPAudioManager::doProfileElection()
{
    bool found = false;
    unsigned pc = 0;

    static QRegularExpression isAudioProfile = QRegularExpression("^audio[0-9]+$");
    QStringList groups = m_settings->childGroups();
    QString playbackHash;
    QString captureHash;
    QString ringHash;

    for (auto &group : std::as_const(groups)) {
        if (isAudioProfile.match(group).hasMatch()) {
            playbackHash = m_settings->value(QString("%1/playback").arg(group)).toString();
            captureHash = m_settings->value(QString("%1/capture").arg(group)).toString();
            ringHash = m_settings->value(QString("%1/ringing").arg(group)).toString();
            m_externalRinger =
                    m_settings->value(QString("%1/preferExternalRinger").arg(group), false)
                            .toBool();

            // Check if all audio hashes are available for this profile
            if (isDeviceAvailable(playbackHash) && isDeviceAvailable(captureHash)
                && isDeviceAvailable(ringHash)) {
                m_playbackHash = playbackHash;
                emit playbackDeviceIdChanged();

                m_captureHash = captureHash;
                emit captureDeviceIdChanged();

                m_ringHash = ringHash;
                emit ringDeviceIdChanged();

                m_currentAudioProfile = pc;
                emit currentProfileChanged();
                found = true;
                break;
            }

            pc++;
        }
    }

    if (found) {
        emit matchingAudioProfile();
    } else {
        m_currentAudioProfile = pc;
        if (SIPManager::instance().isConfigured()) {
            emit noMatchingAudioProfile();
        }

        qCWarning(lcSIPAudioManager) << "no matching audio profile found - using defaults";
    }

    // Load hashes for default devices if nothing is set
    if (m_devices.count() >= 2) {
        auto defaultCaptureDevice = m_devices.first();
        auto defaultPlaybackDevice = m_devices.value(1);

        if (m_playbackHash.isEmpty()) {
            m_playbackHash = defaultPlaybackDevice->uniqueId();
            emit playbackDeviceIdChanged();
        }

        if (m_captureHash.isEmpty()) {
            m_captureHash = defaultCaptureDevice->uniqueId();
            emit captureDeviceIdChanged();
        }

        if (m_ringHash.isEmpty()) {
            m_ringHash = defaultPlaybackDevice->uniqueId();
            emit ringDeviceIdChanged();
        }
    }
}

QAudioOutput *SIPAudioManager::getQtAudioOutputForHash(const QString &id)
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
            qCWarning(lcSIPAudioManager) << "audio device not found - using default";
        }

        dev = new QAudioOutput(QMediaDevices::defaultAudioOutput(), this);
    }

    return dev;
}

QAudioInput *SIPAudioManager::getQtAudioInputForHash(const QString &id)
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
            qCWarning(lcSIPAudioManager) << "audio device not found - using default";
        }

        dev = new QAudioInput(QMediaDevices::defaultAudioInput(), this);
    }

    return dev;
}

bool SIPAudioManager::hasCaptureAudioLevel() const
{
    if (m_captureAudioPort) {
        return m_captureAudioPort->sourceLevel() > 0.4;
    }
    return 0.0;
}

qreal SIPAudioManager::captureAudioVolume() const
{
    if (m_captureAudioPort) {
        const auto source = m_captureAudioPort->audioSource();
        if (source) {
            return source->volume();
        }
    }

    return 0.0;
}

void SIPAudioManager::setCaptureAudioVolume(qreal volume)
{
    Q_ASSERT(0.0 <= volume && volume <= 1.0);

    if (m_captureAudioPort) {
        auto source = m_captureAudioPort->audioSource();
        if (source && source->volume() != volume) {
            source->setVolume(volume);
            emit captureAudioVolumeChanged();
        }
    }
}

qreal SIPAudioManager::playbackAudioVolume() const
{
    if (m_playbackAudioPort) {
        const auto sink = m_playbackAudioPort->audioSink();
        if (sink) {
            return sink->volume();
        }
    }
    return 0.0;
}

void SIPAudioManager::setPlaybackAudioVolume(qreal volume)
{
    Q_ASSERT(0.0 <= volume && volume <= 1.0);

    if (m_playbackAudioPort) {
        auto sink = m_playbackAudioPort->audioSink();

        if (sink && sink->volume() != volume) {
            sink->setVolume(volume);
            emit playbackAudioVolumeChanged();
        }
    }
}
