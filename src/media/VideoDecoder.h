#pragma once

#include "LibAvPointers.h"

#include <QByteArrayView>
#include <QList>
#include <QSize>
#include <QString>
#include <QVideoFrame>

#include <expected>
#include <optional>

/// Decodes WebRTC video frames into I420
class VideoDecoder
{
    Q_DISABLE_COPY_MOVE(VideoDecoder)

public:
    enum class Codec { VP8, VP9, AV1 };

    VideoDecoder() = default;

    static bool isSupported(Codec codec);
    static QString codecName(Codec codec);

    std::expected<void, QString> open(Codec codec);
    std::optional<Codec> codec() const { return m_codec; }

    /// Expects one complete encoded frame; may return no frame (e.g. hidden frames)
    std::expected<QList<QVideoFrame>, QString> decode(QByteArrayView payload);

private:
    std::expected<QVideoFrame, QString> toVideoFrame(AVFramePtr frame);
    std::expected<AVFramePtr, QString> convertToYuv420(const AVFrame &frame);

    AVCodecContextPtr m_context;
    AVPacketPtr m_packet;
    SwsContextPtr m_scaler;
    AVPixelFormat m_scalerFormat = AV_PIX_FMT_NONE;
    QSize m_scalerSize;
    std::optional<Codec> m_codec;
};
