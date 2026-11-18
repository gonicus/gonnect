#pragma once

#include <QSize>
#include <QString>
#include <QVideoFrame>
#include <QVideoFrameFormat>

#include <spa/buffer/buffer.h>
#include <spa/param/video/raw.h>

#include "LibAvPointers.h"

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <array>
#include <memory>
#include <optional>
#include <vector>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

// Converts PipeWire video buffers (raw or MJPEG) into I420
class PipeWireFrameConverter
{

public:
    bool setFormat(const spa_video_info_raw &info);
    bool setMjpegFormat();
    void reset();

    bool isValid() const;

    // Missing, corrupted or short buffer -> return nothing
    std::optional<QVideoFrame> convert(const spa_buffer &buffer);

    // Short name (e.g. "YUV2")
    static QString formatName(spa_video_format format);

private:
    bool createScaler(AVPixelFormat sourceFormat, const QSize &size);
    bool updateScaler(const AVFrame &frame);
    std::optional<QVideoFrame> convertRaw(const uint8_t *bytes, qsizetype size,
                                          int32_t chunkStride);
    std::optional<QVideoFrame> convertMjpeg(const uint8_t *bytes, qsizetype size);
    std::optional<QVideoFrame> scale(const uint8_t *const planes[], const int strides[]);

    std::array<int, 4> sourceStrides(int32_t chunkStride) const;

    AVPixelFormat m_sourceFormat = AV_PIX_FMT_NONE;
    QSize m_size;
    QVideoFrameFormat m_frameFormat;
    std::unique_ptr<SwsContext, SwsContextDeleter> m_scaler;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> m_decoder;
    std::unique_ptr<AVFrame, AVFrameDeleter> m_decodedFrame;
    std::unique_ptr<AVPacket, AVPacketDeleter> m_packet;
    std::vector<uint8_t> m_packetData;
};
