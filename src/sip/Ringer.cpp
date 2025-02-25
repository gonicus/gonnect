#include <QLoggingCategory>
#include <QMediaDevices>
#include <QAudioOutput>
#include <QAudioDevice>

#include "SIPAudioManager.h"
#include "Ringer.h"
#include "AppSettings.h"
#include "USBDevices.h"
#include "HeadsetDeviceProxy.h"
#include "SystemTrayMenu.h"

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
    auto currentProfile = SIPAudioManager::instance().currentProfile();

    SystemTrayMenu::instance().setRinging(true);

    // Prefer headset ringer?
    if (settings.value(QString("audio%1/preferExternalRinger").arg(currentProfile), false)
                .toBool()) {
        auto proxy = USBDevices::instance().getHeadsetDeviceProxy();
        if (proxy->available()) {
            proxy->setRing(true);
            return;
        }
    }

    if (!m_player) {
        m_player = new QMediaPlayer(this);
        connect(m_player, &QMediaPlayer::playbackStateChanged, this, &Ringer::playbackStateChanged);
    }

    // Determine ring tone path
    auto source = settings.value(QString("audio%1/ringtone").arg(currentProfile), "").toString();
    if (source.isEmpty()) {
        source = "qrc:/audio/phone-incoming-call.oga";
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
            qCCritical(lcRinger) << "unknown ringing device in config - using default";
            dev = new QAudioOutput(QMediaDevices::defaultAudioOutput(), m_player);
        }

    } else {
        dev = new QAudioOutput(QMediaDevices::defaultAudioOutput(), m_player);
    }

    m_player->setAudioOutput(dev);
    dev->setVolume(
            (0.0 <= customVolume && customVolume <= 1.0)
                    ? customVolume
                    : settings.value(QString("audio%1/ringtoneVolume").arg(currentProfile), 90.0)
                                    .toFloat()
                            / 100.0);

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
    auto proxy = USBDevices::instance().getHeadsetDeviceProxy();
    proxy->setRing(false);

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
