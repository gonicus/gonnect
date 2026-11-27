#include "KeyFrameGate.h"

bool KeyFrameGate::accept(const EncodedFrame &frame)
{
    if (m_codec != frame.codec) {
        m_codec = frame.codec;
        m_isWaitingForKeyFrame = true;
    }

    if (m_isWaitingForKeyFrame && !frame.isKeyFrame) {
        return false;
    }

    m_isWaitingForKeyFrame = false;
    return true;
}

bool KeyFrameGate::shouldRequestKeyFrame(std::chrono::steady_clock::time_point now)
{
    if (!m_isWaitingForKeyFrame) {
        return false;
    }

    if (m_lastRequest && now - *m_lastRequest < RequestInterval) {
        return false;
    }

    m_lastRequest = now;
    return true;
}
