#include "CameraFrameSerializer.h"

#include <QImage>
#include <QtEndian>

namespace {

using FrameFormat = CameraFrameSerializer::FrameFormat;

void appendUint32(QByteArray &data, quint32 value)
{
    const quint32 littleEndian = qToLittleEndian(value);
    data.append(reinterpret_cast<const char *>(&littleEndian), sizeof(littleEndian));
}

void appendHeader(QByteArray &data, FrameFormat format, int width, int height)
{
    appendUint32(data, static_cast<quint32>(format));
    appendUint32(data, static_cast<quint32>(width));
    appendUint32(data, static_cast<quint32>(height));
}

void appendRows(QByteArray &data, const uchar *bits, qsizetype bytesPerLine, int rowLength,
                int rowCount)
{
    for (int y = 0; y < rowCount; ++y) {
        data.append(reinterpret_cast<const char *>(bits + y * bytesPerLine), rowLength);
    }
}

QByteArray serializeI420(QVideoFrame frame)
{
    if (!frame.map(QVideoFrame::ReadOnly)) {
        return {};
    }

    const int width = frame.width();
    const int height = frame.height();
    const int chromaWidth = (width + 1) / 2;
    const int chromaHeight = (height + 1) / 2;

    QByteArray data;
    data.reserve(12 + width * height + 2 * chromaWidth * chromaHeight);
    appendHeader(data, FrameFormat::I420, width, height);
    appendRows(data, frame.bits(0), frame.bytesPerLine(0), width, height);
    appendRows(data, frame.bits(1), frame.bytesPerLine(1), chromaWidth, chromaHeight);
    appendRows(data, frame.bits(2), frame.bytesPerLine(2), chromaWidth, chromaHeight);

    frame.unmap();
    return data;
}

QByteArray serializeRgbx(const QVideoFrame &frame)
{
    const QImage image = frame.toImage().convertToFormat(QImage::Format_RGBX8888);
    if (image.isNull()) {
        return {};
    }

    QByteArray data;
    data.reserve(12 + image.width() * image.height() * 4);
    appendHeader(data, FrameFormat::RGBX, image.width(), image.height());
    appendRows(data, image.constBits(), image.bytesPerLine(), image.width() * 4, image.height());
    return data;
}
} // namespace

QByteArray CameraFrameSerializer::serialize(const QVideoFrame &frame)
{
    if (frame.pixelFormat() == QVideoFrameFormat::Format_YUV420P) {
        return serializeI420(frame);
    }

    return serializeRgbx(frame);
}
