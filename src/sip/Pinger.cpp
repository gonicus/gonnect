#include "Pinger.h"
#include "AppSettings.h"
#include "AudioManager.h"
#include "AudioHelper.h"

#include <QAudioOutput>
#include <QLoggingCategory>
#include <QMediaDevices>
#include <QAudioDevice>

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

    // Setup player and volume
    QString ringerHash = settings.value(QString("audio%1/ringing").arg(currentProfile)).toString();
    QAudioOutput *dev = AudioHelper::createAudioOutput(ringerHash, m_player);

    m_player->setAudioOutput(dev);

    qreal configVol =
            settings.value(QString("audio%1/notificationVolume").arg(currentProfile), 90.0)
                    .toFloat();
    dev->setVolume(AudioHelper::calculateVolume(customVolume, configVol));

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
