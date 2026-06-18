#include "Pinger.h"
#include "AppSettings.h"
#include "AudioManager.h"

#include <QAudioOutput>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPinger, "gonnect.sip.pinger")

Pinger::Pinger(QObject *parent) : QObject{ parent } { }

Pinger::~Pinger()
{
    stop();
}

void Pinger::ping(qreal customVolume)
{
    AppSettings settings;
    auto currentProfile = AudioManager::instance().currentProfile();

    // Load sound file path
    const auto source =
            settings.value(QString("audio%1/notificationTone").arg(currentProfile), "").toString();

    if (source.isEmpty()) {
        Q_EMIT stopped();
        return;
    }

    if (!m_player) {
        m_player = new QMediaPlayer(this);
        connect(m_player, &QMediaPlayer::playbackStateChanged, this,
                &Pinger::onPlaybackStateChanged);
    }

    // Setup ringing output device
    QString ringerHash = settings.value(QString("audio%1/ringing").arg(currentProfile)).toString();
    QAudioOutput *dev = nullptr;

    if (!ringerHash.isEmpty()) {
        const QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();
        for (const QAudioDevice &device : audioDevices) {
            auto calculatedHash = SIPAudioDevice::makeHash(device.description(), false);
            if (calculatedHash == ringerHash) {
                dev = new QAudioOutput(device, m_player);
                break;
            }
        }

        if (!dev) {
            qCCritical(lcPinger) << "unknown ringing device in config - using default";
            dev = new QAudioOutput(QMediaDevices::defaultAudioOutput(), m_player);
        }

    } else {
        dev = new QAudioOutput(QMediaDevices::defaultAudioOutput(), m_player);
    }

    m_player->setAudioOutput(dev);
    dev->setVolume(
            (0.0 <= customVolume && customVolume <= 1.0)
                    ? customVolume
                    : settings.value(QString("audio%1/notificationVolume").arg(currentProfile),
                                     90.0)
                                    .toFloat()
                            / 100.0);

    m_player->setSource(source);
    m_player->play();
}

void Pinger::stop()
{
    if (m_player) {
        // Make stop() safe against multiple calls
        auto *player = m_player;
        m_player = nullptr;
        player->stop();
        player->deleteLater();

        Q_EMIT stopped();
    }
}

void Pinger::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState) {
        stop();
    }
}
