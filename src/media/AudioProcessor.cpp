#include <QLoggingCategory>
#include <pjsua-lib/pjsua.h>
#include <pjmedia/echo.h>

#include "AudioProcessor.h"

Q_LOGGING_CATEGORY(lcAudioProcessor, "gonnect.sip.audio.processor")

AudioProcessor::AudioProcessor(unsigned clockRate, unsigned channelCount, unsigned samplesPerFrame,
                               unsigned tailMs, unsigned features)
{
    m_frameBytes = samplesPerFrame * channelCount * sizeof(pj_int16_t);
    m_echoCancellation = features & EchoCancellation;
    m_gainControl = features & GainControl;
    m_silence.assign(samplesPerFrame * channelCount, 0);

    if (tailMs == 0) {
        qCWarning(lcAudioProcessor) << "tail length must be > 0, echo canceller disabled";
        return;
    }

    // AGC/AEC/ANC is only implemented in the WebRTC backend: select WebRTC AEC3
    unsigned options = PJMEDIA_ECHO_WEBRTC_AEC3;
    if (features & NoiseSuppression) {
        options |= PJMEDIA_ECHO_USE_NOISE_SUPPRESSOR;
    }
    if (features & GainControl) {
        options |= PJMEDIA_ECHO_USE_GAIN_CONTROLLER;
    }

    m_pool = pjsua_pool_create("ec", 512, 512);
    if (!m_pool) {
        qCWarning(lcAudioProcessor) << "failed to create memory pool";
        return;
    }

    pj_status_t status = pjmedia_echo_create2(m_pool, clockRate, channelCount, samplesPerFrame,
                                              tailMs, 0 /* latency unknown */, options, &m_ec);
    if (status != PJ_SUCCESS) {
        char errbuf[128];
        pj_strerror(status, errbuf, sizeof(errbuf));
        qCWarning(lcAudioProcessor) << "pjmedia_echo_create2 failed:" << errbuf;
        m_ec = nullptr;
        pj_pool_release(m_pool);
        m_pool = nullptr;
        return;
    }

    qCInfo(lcAudioProcessor).nospace()
            << "AudioProcessor initialized (clockRate=" << clockRate
            << ", channels=" << channelCount << ", samplesPerFrame=" << samplesPerFrame
            << ", tailMs=" << tailMs << ", options=0x" << Qt::hex << options << ")";
}

AudioProcessor::~AudioProcessor()
{
    if (m_ec) {
        pjmedia_echo_destroy(m_ec);
        m_ec = nullptr;
    }
    if (m_pool) {
        pj_pool_release(m_pool);
        m_pool = nullptr;
    }
}

void AudioProcessor::capture(void *buf, unsigned size)
{
    if (!m_ec || !buf || size != m_frameBytes) {
        return;
    }

    // Only feed playback as a reference when AEC is active
    if (!m_echoCancellation) {
        pjmedia_echo_playback(m_ec, reinterpret_cast<pj_int16_t *>(m_silence.data()));
    }

    pjmedia_echo_capture(m_ec, static_cast<pj_int16_t *>(buf), 0);
}

void AudioProcessor::playback(void *buf, unsigned size)
{
    if (m_echoCancellation && m_ec && buf && size == m_frameBytes) {
        pjmedia_echo_playback(m_ec, static_cast<pj_int16_t *>(buf));
    }
}
