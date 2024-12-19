#pragma once
#include <QObject>
#include <QPointer>
#include <pjsua2.hpp>

class SIPCall;

class Sniffer : public QObject, public pj::AudioMediaPort
{
    Q_OBJECT
    Q_DISABLE_COPY(Sniffer)

public:
    explicit Sniffer(SIPCall *parent = nullptr);
    virtual ~Sniffer();

    bool initialize();

    void onFrameRequested(pj::MediaFrame &frame) override;
    void onFrameReceived(pj::MediaFrame &frame) override;

    qreal audioLevel() const { return m_audioLevel; }

signals:
    void audioLevelChanged(qreal level);

private:
    bool initFmt();
    void updateAudioLevel(const char *data, qint64 size);

    SIPCall *m_call = nullptr;

    qreal m_audioLevel = 0.0;

    pj::MediaFormatAudio m_pj_fmt;
};
