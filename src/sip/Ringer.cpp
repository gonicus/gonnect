#include <QLoggingCategory>
#include <QMediaDevices>
#include <QAudioOutput>
#include <QAudioDevice>

#include "AudioManager.h"
#include "Ringer.h"
#include "AppSettings.h"
#include "SystemTrayMenu.h"
#include "AudioHelper.h"

Q_LOGGING_CATEGORY(lcRinger, "gonnect.sip.ringer")

Ringer::Ringer(QObject *parent) : QObject(parent)
{
    m_delayer.setSingleShot(true);
    connect(&m_delayer, &QTimer::timeout, this, [this]() {
        if (m_player) {
            m_player->play();
        }
    });
}

void Ringer::start(qreal customVolume)
{
    AppSettings settings;
    auto currentProfile = AudioManager::instance().currentProfile();

    SystemTrayMenu::instance().setRinging(true);

    // Prefer headset ringer?
    if (settings.value(QString("audio%1/preferExternalRinger").arg(currentProfile), false)
                .toBool()) { }

    if (!m_player) {
        m_player = new QMediaPlayer(this);
        connect(m_player, &QMediaPlayer::playbackStateChanged, this, &Ringer::playbackStateChanged);
    }

    // Determine ring tone path
    auto source = settings.value(QString("audio%1/ringtone").arg(currentProfile), "").toString();
    if (source.isEmpty()) {
        source = "qrc:/audio/phone-incoming-call.mp3";
    }

    if (source.isEmpty()) {
        qCCritical(lcRinger) << "no ring tone specified";
        return;
    }

    // Determine ring tone pause
    bool ok = false;
    m_delay = settings.value(QString("audio%1/ringtonePause").arg(currentProfile),
                             DEFAULT_RINGTONE_DELAY)
                      .toUInt(&ok);
    if (!ok) {
        qCCritical(lcRinger) << "invalid value for ringtonePause in config - using default";
        m_delay = DEFAULT_RINGTONE_DELAY;
    }

    // Setup player and volume
    QString ringerHash = settings.value(QString("audio%1/ringing").arg(currentProfile)).toString();
    QAudioOutput *dev = AudioHelper::createAudioOutput(ringerHash, m_player);

    m_player->setAudioOutput(dev);

    qreal configVol =
            settings.value(QString("audio%1/ringtoneVolume").arg(currentProfile), 90.0).toFloat();
    dev->setVolume(AudioHelper::calculateVolume(customVolume, configVol));

    m_player->setSource(source);
    m_player->play();
}

void Ringer::playbackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState) {

        // Restart after configured pause in ms
        if (m_player) {
            if (m_delay) {
                m_delayer.start(m_delay);
            } else {
                m_player->play();
            }
        }
    }
}

void Ringer::stop()
{
    SystemTrayMenu::instance().setRinging(false);

    m_delayer.stop();

    if (m_player) {
        m_player->stop();
        m_player->deleteLater();
        m_player = nullptr;
    }
}

Ringer::~Ringer()
{
    stop();
}
