#include "PipeWireFrameConverter.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
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

    m_sourceFormat = mapping->avFormat;
    m_size = QSize(static_cast<int>(info.size.width), static_cast<int>(info.size.height));

    m_frameFormat = QVideoFrameFormat(m_size, QVideoFrameFormat::Format_YUV420P);
    m_frameFormat.setColorSpace(QVideoFrameFormat::ColorSpace_BT601);
    m_frameFormat.setColorRange(QVideoFrameFormat::ColorRange_Video);

    m_scaler.reset(sws_getContext(m_size.width(), m_size.height(), m_sourceFormat, m_size.width(),
                                  m_size.height(), AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr,
                                  nullptr, nullptr));

    return m_scaler != nullptr;
}

void PipeWireFrameConverter::reset()
{
    m_scaler.reset();
    m_sourceFormat = AV_PIX_FMT_NONE;
    m_size = QSize();
    m_frameFormat = QVideoFrameFormat();
}

std::optional<QVideoFrame> PipeWireFrameConverter::convert(const spa_buffer &buffer)
{
    if (!m_scaler || buffer.n_datas != 1) {
        return std::nullopt;
    }

    const spa_data &data = buffer.datas[0];
    if (!data.data || !data.chunk || (data.chunk->flags & SPA_CHUNK_FLAG_CORRUPTED)) {
        return std::nullopt;
    }

    const auto strides = sourceStrides(data.chunk->stride);
    std::array<uint8_t *, 4> planes = {};
    auto *base = static_cast<uint8_t *>(data.data) + data.chunk->offset;

    const int requiredBytes = av_image_fill_pointers(planes.data(), m_sourceFormat, m_size.height(),
                                                     base, strides.data());
    const qsizetype filledBytes = std::min(static_cast<qsizetype>(data.chunk->size),
                                           static_cast<qsizetype>(data.maxsize)
                                                   - static_cast<qsizetype>(data.chunk->offset));

    if (requiredBytes < 0 || requiredBytes > filledBytes) {
        return std::nullopt;
    }

    QVideoFrame frame(m_frameFormat);
    if (!frame.map(QVideoFrame::WriteOnly)) {
        return std::nullopt;
    }

    const std::array<uint8_t *, 4> targetPlanes = { frame.bits(0), frame.bits(1), frame.bits(2),
                                                    nullptr };
    const std::array<int, 4> targetStrides = { frame.bytesPerLine(0), frame.bytesPerLine(1),
                                               frame.bytesPerLine(2), 0 };

    const int convertedRows = sws_scale(m_scaler.get(), planes.data(), strides.data(), 0,
                                        m_size.height(), targetPlanes.data(), targetStrides.data());

    frame.unmap();

    if (convertedRows <= 0) {
        return std::nullopt;
    }
    return frame;
}

QString PipeWireFrameConverter::formatName(spa_video_format format)
{
    if (const auto *mapping = findMapping(format)) {
        return QString::fromLatin1(mapping->name);
    }
    return QString::number(static_cast<int>(format));
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

void PipeWireFrameConverter::SwsContextDeleter::operator()(SwsContext *context) const
{
    sws_freeContext(context);
}
