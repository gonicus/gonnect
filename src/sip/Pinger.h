#pragma once

#include <QObject>
#include <QMediaPlayer>

class Pinger : public QObject
{
    Q_OBJECT

public:
    explicit Pinger(QObject *parent = nullptr);
    ~Pinger();

    void ping(qreal customVolume = -1.0);
    void stop();

Q_SIGNALS:
    void stopped();

private Q_SLOTS:
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:
    QMediaPlayer *m_player = nullptr;
};
