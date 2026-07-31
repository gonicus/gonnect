#pragma once
#include <QPointer>
#include <QTimer>
#include <QAudioSink>
#include <QAudioSource>
#include <QAudioFormat>
#include <pjsua2.hpp>

class AudioProcessor;

class AudioPort : public QObject, public pj::AudioMediaPort
{
    Q_OBJECT
    Q_DISABLE_COPY(AudioPort)

public:
    explicit AudioPort(QAudioDevice device);
    virtual ~AudioPort();

    bool initialize();
    void setMuted(bool value);

    // Optional shared microphone audio processor (AGC/AEC/ANC)
    void setAudioProcessor(AudioProcessor *audioProcessor);

    void onFrameRequested(pj::MediaFrame &frame) override;
    void onFrameReceived(pj::MediaFrame &frame) override;

    QString getDeviceID() const;
    QString getSystemDeviceID() const;

    void acquire();
    void release();

    void setAudioDevice(QAudioDevice device);
    QAudioDevice audioDevice() { return m_device; }

    QPointer<QAudioSink> audioSink() { return m_sink; }
    QPointer<QAudioSource> audioSource() { return m_source; }

    qreal sourceLevel() const { return m_sourceAudioLevel; }

    void writeSilenceMS(unsigned milliseconds);

Q_SIGNALS:
    void startIdleTimer();
    void audioSourceChanged();
    void audioSinkChanged();
    void sourceLevelChanged(qreal level);

private:
    void startIO();
    void stopIO();

    void startSinkIO();
    void stopSinkIO();
    void startSourceIO();
    void stopSourceIO();

    bool initFmt();

    float activeTxLevel() const;

    void updateAudioLevel(const char *data, qint64 size);
    void setSourceAudioLevel(qreal level);

    AudioProcessor *m_audioProcessor = nullptr;

    bool m_isMuted = false;
    bool m_isDraining = false;
    bool m_isWarmingUp = false;

    QAudioDevice m_device;

    QPointer<QIODevice> m_io;
    QPointer<QAudioSink> m_sink;
    QPointer<QAudioSource> m_source;

    QTimer m_idleTimer;
    qreal m_sourceAudioLevel = 0.0;
    quint64 m_captureFrames = 0;

    pj::MediaFormatAudio m_pj_fmt;

    QAudioFormat m_audioFormat;

    QMetaObject::Connection m_warmUpDrain;
};
