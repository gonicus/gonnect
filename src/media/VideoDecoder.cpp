#include "VideoDecoder.h"
#include "PlanarVideoBuffer.h"

#include <QScopeGuard>

extern "C" {
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

#include <array>
#include <cstring>

namespace {

const char *decoderName(VideoDecoder::Codec codec)
{
    switch (codec) {

    case VideoDecoder::Codec::VP8:
        return "vp8";
    case VideoDecoder::Codec::VP9:
        return "vp9";
    case VideoDecoder::Codec::AV1:
        return "libav1d";
    }

    return nullptr;
}

QString errorString(int error)
{
    std::array<char, AV_ERROR_MAX_STRING_SIZE> message = {};
    av_strerror(error, message.data(), message.size());
    return QString::fromUtf8(message.data());
}

bool isYuv420(int format)
{
    return format == AV_PIX_FMT_YUV420P || format == AV_PIX_FMT_YUVJ420P;
}

} // namespace

bool VideoDecoder::isSupported(Codec codec)
{
    return avcodec_find_decoder_by_name(decoderName(codec)) != nullptr;
}

QString VideoDecoder::codecName(Codec codec)
{
    switch (codec) {

    case Codec::VP8:
        return QStringLiteral("VP8");
    case Codec::VP9:
        return QStringLiteral("VP9");
    case Codec::AV1:
        return QStringLiteral("AV1");
    }
    return {};
}

std::expected<void, QString> VideoDecoder::open(Codec codec)
{
    m_context.reset();
    m_codec.reset();

    const auto *decoder = avcodec_find_decoder_by_name(decoderName(codec));
    if (!decoder) {
        return std::unexpected(QStringLiteral("No decoder for %1").arg(codecName(codec)));
    }

    AVCodecContextPtr context(avcodec_alloc_context3(decoder));
    if (!m_packet) {
        m_packet.reset(av_packet_alloc());
    }
    if (!context || !m_packet) {
        return std::unexpected(QStringLiteral("Out of memory"));
    }

    // Every frame must leave decoder immediately (real-time video)
    context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    context->thread_count = 1;
    if (codec == Codec::AV1) {
        av_opt_set_int(context->priv_data, "max_frame_delay", 1, 0);
    }

    const int result = avcodec_open2(context.get(), decoder, nullptr);
    if (result < 0) {
        return std::unexpected(QStringLiteral("Cannot open %1 deocder: %2")
                                       .arg(codecName(codec), errorString(result)));
    }

    m_context = std::move(context);
    m_codec = codec;
    return {};
}

std::expected<QList<QVideoFrame>, QString> VideoDecoder::decode(QByteArrayView payload)
{
    if (!m_context) {
        return std::unexpected(QStringLiteral("Decoder not open"));
    }
    if (payload.isEmpty()) {
        return QList<QVideoFrame>();
    }

    // Add zeroed padding (decoders rely on it)
    if (av_new_packet(m_packet.get(), static_cast<int>(payload.size())) < 0) {
        return std::unexpected(QStringLiteral("Out of memory"));
    }

    const auto unrefPacket = qScopeGuard([this]() { av_packet_unref(m_packet.get()); });
    std::memcpy(m_packet->data, payload.data(), static_cast<size_t>(payload.size()));

    const int sendResult = avcodec_send_packet(m_context.get(), m_packet.get());
    if (sendResult < 0) {
        return std::unexpected(QStringLiteral("Cannot decode: %1").arg(errorString(sendResult)));
    }

    QList<QVideoFrame> frames;
    while (true) {
        AVFramePtr frame(av_frame_alloc());
        if (!frame) {
            return std::unexpected(QStringLiteral("Out of memory"));
        }

        const int receivedResult = avcodec_receive_frame(m_context.get(), frame.get());
        if (receivedResult == AVERROR(EAGAIN) || receivedResult == AVERROR_EOF) {
            return frames;
        }
        if (receivedResult < 0) {
            return std::unexpected(
                    QStringLiteral("Cannot decode: %1").arg(errorString(receivedResult)));
        }

        auto videoFrame = toVideoFrame(std::move(frame));
        if (!videoFrame) {
            return std::unexpected(videoFrame.error());
        }
        frames.append(*videoFrame);
    }
}

std::expected<QVideoFrame, QString> VideoDecoder::toVideoFrame(AVFramePtr frame)
{
    if (!isYuv420(frame->format)) {
        auto converted = convertToYuv420(*frame);
        if (!converted) {
            return std::unexpected(converted.error());
        }
        frame = std::move(*converted);
    }
    return QVideoFrame(std::make_unique<PlanarVideoBuffer>(std::move(frame)));
}

std::expected<AVFramePtr, QString> VideoDecoder::convertToYuv420(const AVFrame &frame)
{
    const auto sourceFormat = static_cast<AVPixelFormat>(frame.format);
    const QSize size(frame.width, frame.height);

    if (!m_scaler || sourceFormat != m_scalerFormat || size != m_scalerSize) {
        m_scaler.reset(sws_getContext(frame.width, frame.height, sourceFormat, frame.width,
                                      frame.height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr,
                                      nullptr, nullptr));
        if (!m_scaler) {
            return std::unexpected(QStringLiteral("Cannot convert from %1")
                                           .arg(av_get_pix_fmt_name(sourceFormat)));
        }
        m_scalerFormat = sourceFormat;
        m_scalerSize = size;
    }

    AVFramePtr converted(av_frame_alloc());
    if (!converted) {
        return std::unexpected(QStringLiteral("Out of memory"));
    }
    converted->format = AV_PIX_FMT_YUV420P;
    converted->width = frame.width;
    converted->height = frame.height;
    converted->colorspace = frame.colorspace;
    converted->color_range = frame.color_range;

    if (av_frame_get_buffer(converted.get(), 0) < 0) {
        return std::unexpected(QStringLiteral("Out of memory"));
    }

    const int convertedRows = sws_scale(m_scaler.get(), frame.data, frame.linesize, 0, frame.height,
                                        converted->data, converted->linesize);
    if (convertedRows != frame.height) {
        return std::unexpected(
                QStringLiteral("Cannot convert from %1").arg(av_get_pix_fmt_name(sourceFormat)));
    }

    return converted;
}
