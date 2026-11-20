#include "PipeWireFrameConverter.h"

#include <QScopeGuard>

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
}

#include <algorithm>
#include <cstring>
#include <memory>

namespace {

struct FormatMapping
{
    spa_video_format spaFormat;
    AVPixelFormat avFormat;
    const char *name;
};

constexpr std::array FormatMappings = {
    FormatMapping{ SPA_VIDEO_FORMAT_YUY2, AV_PIX_FMT_YUYV422, "YUY2" },
    FormatMapping{ SPA_VIDEO_FORMAT_NV12, AV_PIX_FMT_NV12, "NV12" },
    FormatMapping{ SPA_VIDEO_FORMAT_I420, AV_PIX_FMT_YUV420P, "I420" },
    FormatMapping{ SPA_VIDEO_FORMAT_BGRx, AV_PIX_FMT_BGR0, "BGRx" },
    FormatMapping{ SPA_VIDEO_FORMAT_GRAY8, AV_PIX_FMT_GRAY8, "GRAY8" },
};

const FormatMapping *findMapping(spa_video_format format)
{
    const auto it = std::ranges::find(FormatMappings, format, &FormatMapping::spaFormat);
    return it != FormatMappings.end() ? std::to_address(it) : nullptr;
}

} // namespace

bool PipeWireFrameConverter::setFormat(const spa_video_info_raw &info)
{
    reset();

    const auto *mapping = findMapping(info.format);
    if (!mapping) {
        return false;
    }

    return createScaler(
            mapping->avFormat,
            QSize(static_cast<int>(info.size.width), static_cast<int>(info.size.height)));
}

bool PipeWireFrameConverter::setMjpegFormat()
{
    reset();
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    if (!codec) {
        return false;
    }

    m_decoder.reset(avcodec_alloc_context3(codec));
    m_decodedFrame.reset(av_frame_alloc());
    m_packet.reset(av_packet_alloc());

    if (!m_decoder || !m_decodedFrame || !m_packet
        || avcodec_open2(m_decoder.get(), codec, nullptr) < 0) {
        reset();
        return false;
    }

    return true;
}

void PipeWireFrameConverter::reset()
{
    m_scaler.reset();
    m_sourceFormat = AV_PIX_FMT_NONE;
    m_size = QSize();
    m_frameFormat = QVideoFrameFormat();

    m_decoder.reset();
    m_decodedFrame.reset();
    m_packet.reset();
    m_packetData.clear();
}

bool PipeWireFrameConverter::isMjpegSupported()
{
    return avcodec_find_decoder(AV_CODEC_ID_MJPEG) != nullptr;
}

bool PipeWireFrameConverter::isValid() const
{
    return m_scaler != nullptr || m_decoder != nullptr;
}

std::optional<QVideoFrame> PipeWireFrameConverter::convert(const spa_buffer &buffer)
{
    if (!isValid() || buffer.n_datas != 1) {
        return std::nullopt;
    }

    const spa_data &data = buffer.datas[0];
    if (!data.data || !data.chunk || (data.chunk->flags & SPA_CHUNK_FLAG_CORRUPTED)
        || data.chunk->offset > data.maxsize) {
        return std::nullopt;
    }

    const auto *bytes = static_cast<const uint8_t *>(data.data) + data.chunk->offset;
    const qsizetype size = std::min(static_cast<qsizetype>(data.chunk->size),
                                    static_cast<qsizetype>(data.maxsize - data.chunk->offset));

    return m_decoder ? convertMjpeg(bytes, size) : convertRaw(bytes, size, data.chunk->stride);
}

QString PipeWireFrameConverter::formatName(spa_video_format format)
{
    if (const auto *mapping = findMapping(format)) {
        return QString::fromLatin1(mapping->name);
    }
    return QString::number(static_cast<int>(format));
}

bool PipeWireFrameConverter::createScaler(AVPixelFormat sourceFormat, const QSize &size)
{
    m_sourceFormat = sourceFormat;
    m_size = size;

    m_frameFormat = QVideoFrameFormat(m_size, QVideoFrameFormat::Format_YUV420P);
    m_frameFormat.setColorSpace(QVideoFrameFormat::ColorSpace_BT601);
    m_frameFormat.setColorRange(QVideoFrameFormat::ColorRange_Video);

    m_scaler.reset(sws_getContext(m_size.width(), m_size.height(), m_sourceFormat, m_size.width(),
                                  m_size.height(), AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr,
                                  nullptr, nullptr));

    return m_scaler != nullptr;
}

bool PipeWireFrameConverter::updateScaler(const AVFrame &frame)
{
    const auto format = static_cast<AVPixelFormat>(frame.format);
    const QSize size(frame.width, frame.height);

    if (m_scaler && format == m_sourceFormat && size == m_size) {
        return true;
    }

    if (!createScaler(format, size)) {
        return false;
    }

    const int sourceRange = frame.color_range == AVCOL_RANGE_JPEG ? 1 : 0;
    const int *coefficients = sws_getCoefficients(SWS_CS_ITU601);
    sws_setColorspaceDetails(m_scaler.get(), coefficients, sourceRange, coefficients, 0, 0, 1 << 16,
                             1 << 16);
    return true;
}

std::optional<QVideoFrame> PipeWireFrameConverter::convertRaw(const uint8_t *bytes, qsizetype size,
                                                              int32_t chunkStride)
{
    const auto strides = sourceStrides(chunkStride);
    std::array<uint8_t *, 4> planes = {};
    const int requiredBytes = av_image_fill_pointers(planes.data(), m_sourceFormat, m_size.height(),
                                                     const_cast<uint8_t *>(bytes), strides.data());
    if (requiredBytes < 0 || requiredBytes > size) {
        return std::nullopt;
    }

    return scale(planes.data(), strides.data());
}

std::optional<QVideoFrame> PipeWireFrameConverter::convertMjpeg(const uint8_t *bytes,
                                                                qsizetype size)
{
    m_packetData.resize(static_cast<size_t>(size) + AV_INPUT_BUFFER_PADDING_SIZE);
    std::memcpy(m_packetData.data(), bytes, static_cast<size_t>(size));
    std::memset(m_packetData.data() + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    m_packet->data = m_packetData.data();
    m_packet->size = static_cast<int>(size);

    if (avcodec_send_packet(m_decoder.get(), m_packet.get()) < 0
        || avcodec_receive_frame(m_decoder.get(), m_decodedFrame.get()) < 0) {
        return std::nullopt;
    }

    const auto unrefFrame = qScopeGuard([this]() { av_frame_unref(m_decodedFrame.get()); });

    if (!updateScaler(*m_decodedFrame)) {
        return std::nullopt;
    }

    return scale(m_decodedFrame->data, m_decodedFrame->linesize);
}

std::optional<QVideoFrame> PipeWireFrameConverter::scale(const uint8_t *const planes[],
                                                         const int strides[])

{
    QVideoFrame frame(m_frameFormat);
    if (!frame.map(QVideoFrame::WriteOnly)) {
        return std::nullopt;
    }

    const std::array<uint8_t *, 4> targetPlanes = { frame.bits(0), frame.bits(1), frame.bits(2),
                                                    nullptr };
    const std::array<int, 4> targetStrides = { frame.bytesPerLine(0), frame.bytesPerLine(1),
                                               frame.bytesPerLine(2), 0 };

    const int convertedRows = sws_scale(m_scaler.get(), planes, strides, 0, m_size.height(),
                                        targetPlanes.data(), targetStrides.data());

    frame.unmap();

    if (convertedRows <= 0) {
        return std::nullopt;
    }
    return frame;
}

// The chunk only tells the stride of the first plane, including any padding. Widening the image
// accordingly lets FFmpeg derive the matching strides of all other planes.
std::array<int, 4> PipeWireFrameConverter::sourceStrides(int32_t chunkStride) const
{
    std::array<int, 4> strides = {};
    av_image_fill_linesizes(strides.data(), m_sourceFormat, m_size.width());

    if (chunkStride > 0 && chunkStride != strides[0]) {
        const int paddedWidth = m_size.width() * chunkStride / strides[0];
        av_image_fill_linesizes(strides.data(), m_sourceFormat, paddedWidth);
    }

    return strides;
}
