#pragma once

#include "EncodedFrame.h"

#include <chrono>
#include <optional>

/// Frames passeed only beginning from key frame
class KeyFrameGate
{

public:
    using Clock = std::chrono::steady_clock;

    static constexpr std::chrono::milliseconds RequestInterval{ 500 };

    /// Codec change required new key frame
    bool accept(const EncodedFrame &frame);

    void requireKeyFrame() { m_isWaitingForKeyFrame = true; }
    bool isWaitingForKeyFrame() const { return m_isWaitingForKeyFrame; }

    /// Throttles key frame requests to the sender while waiting
    bool shouldRequestKeyFrame(Clock::time_point now);

private:
    bool m_isWaitingForKeyFrame = true;
    std::optional<VideoCodec> m_codec;
    std::optional<Clock::time_point> m_lastRequest;
};
