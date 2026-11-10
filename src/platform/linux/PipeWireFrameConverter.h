#pragma once

#include <QSize>
#include <QString>
#include <QVideoFrame>
#include <QVideoFrameFormat>

#include <spa/buffer/buffer.h>
#include <spa/param/video/raw.h>

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <array>
#include <memory>
#include <optional>

struct SwsContext;

// Converts PipeWire video buffers into I420
class PipeWireFrameConverter
{

public:
    bool setFormat(const spa_video_info_raw &info);
    void reset();

    bool isValid() const { return m_scaler != nullptr; }

    // Missing, corrupted or short buffer -> return nothing
    std::optional<QVideoFrame> convert(const spa_buffer &buffer);

    // Short name (e.g. "YUV2")
    static QString formatName(spa_video_format format);

private:
    struct SwsContextDeleter
    {
        void operator()(SwsContext *context) const;
    };

    std::array<int, 4> sourceStrides(int32_t chunkStride) const;

    AVPixelFormat m_sourceFormat = AV_PIX_FMT_NONE;
    QSize m_size;
    QVideoFrameFormat m_frameFormat;
    std::unique_ptr<SwsContext, SwsContextDeleter> m_scaler;
};
