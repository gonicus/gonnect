#pragma once
#include <QUrl>
#include <QObject>
#include <QTimer>
#include <QMediaPlayer>

#define DEFAULT_RINGTONE_DELAY 2500

class Ringer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Ringer)

public:
    explicit Ringer(QObject *parent = nullptr);
    ~Ringer();

    void start(qreal customVolume = -1.0);
    void stop();

private slots:
    void playbackStateChanged(QMediaPlayer::PlaybackState state);

private:
    QMediaPlayer *m_player = nullptr;

    QTimer m_delayer;
    qreal m_volumeOverride = -1.0;
    unsigned m_delay = DEFAULT_RINGTONE_DELAY;
};
