#pragma once

#include <cstdint>
#include <vector>

struct pjmedia_echo_state;
struct pj_pool_t;

class AudioProcessor
{
public:
    enum Feature {
        NoiseSuppression = 1 << 0,
        EchoCancellation = 1 << 1,
        GainControl = 1 << 2,
    };

    AudioProcessor(unsigned clockRate, unsigned channelCount, unsigned samplesPerFrame,
                   unsigned tailMs, unsigned features);
    ~AudioProcessor();

    AudioProcessor(const AudioProcessor &) = delete;
    AudioProcessor &operator=(const AudioProcessor &) = delete;

    bool isValid() const { return m_ec != nullptr; }

    bool hasGainControl() const { return m_gainControl; }
    unsigned frameBytes() const { return m_frameBytes; }

    void capture(void *buf, unsigned size);
    void playback(void *buf, unsigned size);

private:
    pj_pool_t *m_pool = nullptr;
    pjmedia_echo_state *m_ec = nullptr;
    unsigned m_frameBytes = 0;
    bool m_echoCancellation = false;
    bool m_gainControl = false;
    std::vector<int16_t> m_silence;
};
