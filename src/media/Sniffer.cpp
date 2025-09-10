#include <QLoggingCategory>
#include <pjmedia/port.h>
#include "SIPCall.h"
#include "Sniffer.h"

Q_LOGGING_CATEGORY(lcSniffer, "gonnect.sip.audio")

using namespace std::chrono_literals;

Sniffer::Sniffer(SIPCall *parent) : QObject(qobject_cast<QObject *>(parent)), m_call(parent) { }

Sniffer::~Sniffer() { }

bool Sniffer::initialize()
{
    if (!initFmt()) {
        // Could not find a supported format for the audio device
        return false;
    }

    createPort(m_call->getInfo().callIdString, m_pj_fmt);

    return true;
}

bool Sniffer::initFmt()
{
    // By default assume 16 bit signed integer linear PCM with 16kHz sampling
    // rate and 20ms frame time. These settings are identical to the
    // implementation of the G.722 codec in pjsip, see
    // https://docs.pjsip.org/en/latest/api/generated/pjmedia/group/group__PJMED__G722.html
    pj_uint32_t formatID = PJMEDIA_FORMAT_L16;
    unsigned int clockRate = 16000;
    unsigned int channelCount = 1;
    int frameTimeUsec = 20000;
    int bitsPerSample = 16;

    m_pj_fmt.init(formatID, clockRate, channelCount, frameTimeUsec, bitsPerSample);

    return true;
}

void Sniffer::updateAudioLevel(const char *data, qint64 size)
{
    qreal max = 0;
    const qint64 numSamples = size / sizeof(qint16);

    static constexpr qreal positiveRange = std::numeric_limits<qint16>().max();

    for (int i = 0; i < numSamples; ++i) { // i is index of sample
        const auto word = static_cast<qint16>(*data);
        const qreal realValue = static_cast<qreal>(word) / positiveRange * 100.0;

        max = std::max(max, realValue);
        data += sizeof(qint16);
    }

    if (m_audioLevel != max) {
        m_audioLevel = max;
        Q_EMIT audioLevelChanged(max);
    }
}

void Sniffer::onFrameRequested(pj::MediaFrame &frame)
{
    Q_UNUSED(frame)
}

void Sniffer::onFrameReceived(pj::MediaFrame &frame)
{
    updateAudioLevel(reinterpret_cast<char *>(frame.buf.data()), frame.size);
}
