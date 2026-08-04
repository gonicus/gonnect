#include "RingTone.h"
#include "AudioManager.h"
#include "AudioPort.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcRingTone, "gonnect.sip.ringtone")

RingTone::RingTone(quint16 frequency1, quint16 frequency2, QList<QPair<quint16, quint16>> intervals,
                   qint8 loopIndex, QObject *parent)
    : QObject(parent),
      m_mediaSink(AudioManager::instance().getPlaybackDevMedia()),
      m_frequency1(frequency1),
      m_frequency2(frequency2),
      m_loopIndex(loopIndex),
      m_intervals(intervals)
{

    try {
        m_toneGen.createToneGenerator();
    } catch (const pj::Error &err) {
        qCCritical(lcRingTone) << "failed to create ring tone generator:" << err.info();
    }

    m_loopTimer.callOnTimeout(this, &RingTone::playNextTone);
    m_loopTimer.setSingleShot(true);

    m_stopTimer.callOnTimeout(this, &RingTone::stop);
    m_stopTimer.setSingleShot(true);
}

RingTone::~RingTone()
{
    stop();
}

void RingTone::start(quint8 times)
{
    m_repeatTimes = times;
    start();
}

void RingTone::start()
{
    if (m_isPlaying) {
        return;
    }
    m_isPlaying = true;
    m_currentIndex = 0;

    AudioManager::instance().acquireDevice();

    if (m_stopTimer.isActive()) {
        m_stopTimer.stop();
    }

    // Bridge gap between previous audio source (mostly the call)
    if (auto *port = dynamic_cast<AudioPort *>(&m_mediaSink)) {
        port->writeSilenceMS(120);
    }

    try {
        m_toneGen.startTransmit(m_mediaSink);
    } catch (const pj::Error &err) {
        qCWarning(lcRingTone) << "failed to start ring tone transmission:" << err.info();
    }
    playNextTone();
}

void RingTone::stop()
{
    m_repeatTimes = -1;

    if (!m_isPlaying) {
        return;
    }

    if (m_loopTimer.isActive()) {
        m_loopTimer.stop();
    }

    m_currentIndex = 0;
    m_isPlaying = false;
    try {
        m_toneGen.stop();
        m_toneGen.stopTransmit(m_mediaSink);
    } catch (const pj::Error &err) {
        qCWarning(lcRingTone) << "failed to stop ring tone:" << err.info();
    }

    AudioManager::instance().releaseDevice();

    Q_EMIT ready();
}

void RingTone::playNextTone()
{
    // Create and play tone
    const auto &tuple = m_intervals.at(m_currentIndex);

    pj::ToneDescVector tones;
    pj::ToneDesc tone;

    tone.freq1 = m_frequency1;
    tone.freq2 = m_frequency2;
    tone.on_msec = tuple.first;
    tone.off_msec = tuple.second;

    tones.push_back(tone);

    try {
        m_toneGen.play(tones, m_loopIndex >= 0);
    } catch (const pj::Error &err) {
        qCWarning(lcRingTone) << "failed to play ring tone:" << err.info();
    }

    // Restart timer
    ++m_currentIndex;
    if (m_currentIndex >= m_intervals.size()) {
        if (m_loopIndex >= 0) {
            m_currentIndex = m_loopIndex;
            if (m_repeatTimes > 0) {
                --m_repeatTimes;
            } else if (m_repeatTimes == 0) {
                scheduleStop(tuple.first + tuple.second);
                return;
            }
        } else {
            scheduleStop(tuple.first + tuple.second);
            return;
        }
    }

    m_loopTimer.start(tuple.first + tuple.second);
}

void RingTone::scheduleStop(unsigned delay)
{
    static constexpr unsigned PIPELINE_GRACE_MS = 400;

    m_stopTimer.setInterval(delay + PIPELINE_GRACE_MS);
    m_stopTimer.start();
}
