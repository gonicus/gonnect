#include "PlanarVideoBuffer.h"

namespace {
QVideoFrameFormat createFormat(const AVFrame &frame)
{
    QVideoFrameFormat format(QSize(frame.width, frame.height), QVideoFrameFormat::Format_YUV420P);

    // WebRTC streams without color information
    format.setColorSpace(frame.colorspace == AVCOL_SPC_BT709 ? QVideoFrameFormat::ColorSpace_BT709
                                                             : QVideoFrameFormat::ColorSpace_BT601);

    const bool isFullRange =
            frame.color_range == AVCOL_RANGE_JPEG || frame.format == AV_PIX_FMT_YUVJ420P;
    format.setColorRange(isFullRange ? QVideoFrameFormat::ColorRange_Full
                                     : QVideoFrameFormat::ColorRange_Video);
    return format;
}
} // namespace

PlanarVideoBuffer::PlanarVideoBuffer(AVFramePtr frame)
    : m_frame(std::move(frame)), m_format(createFormat(*m_frame))
{
}

QAbstractVideoBuffer::MapData PlanarVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if (mode & QVideoFrame::WriteOnly) {
        return {};
    }

    MapData data;
    data.planeCount = 3;

    const int chromaHeight = (m_frame->height + 1) / 2;
    for (int plane = 0; plane < 3; ++plane) {
        const int planeHeight = plane == 0 ? m_frame->height : chromaHeight;
        data.data[plane] = m_frame->data[plane];
        data.bytesPerLine[plane] = m_frame->linesize[plane];
        data.dataSize[plane] = m_frame->linesize[plane] * planeHeight;
    }
    return data;
}

QVideoFrameFormat PlanarVideoBuffer::format() const
{
    return m_format;
}
