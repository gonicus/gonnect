#include "CameraFrameSerializer.test.h"
#include "CameraFrameSerializer.h"

#include <QAbstractVideoBuffer>
#include <QImage>
#include <QTest>
#include <QVideoFrame>

#include <array>
#include <memory>

namespace {

using FrameFormat = CameraFrameSerializer::FrameFormat;

constexpr char PaddingByte = static_cast<char>(0xff);

// Distinct per plane and position, never equal to the padding byte
char planeValue(int plane, int x, int y)
{
    return static_cast<char>(1 + 100 * plane + 10 * y + x);
}

QSize chromaSize(const QSize &size)
{
    return QSize((size.width() + 1) / 2, (size.height() + 1) / 2);
}

QByteArray planeBytes(int plane, const QSize &size, int stride)
{
    QByteArray bytes;
    for (int y = 0; y < size.height(); ++y) {
        QByteArray row;
        for (int x = 0; x < size.width(); ++x) {
            row += planeValue(plane, x, y);
        }
        row.resize(stride, PaddingByte);
        bytes += row;
    }
    return bytes;
}

// Little endian, deliberately written independent of the implementation
QByteArray uint32Bytes(quint32 value)
{
    QByteArray bytes;
    for (int shift = 0; shift < 32; shift += 8) {
        bytes += static_cast<char>((value >> shift) & 0xff);
    }
    return bytes;
}

QByteArray header(FrameFormat format, const QSize &size)
{
    return uint32Bytes(static_cast<quint32>(format))
            + uint32Bytes(static_cast<quint32>(size.width()))
            + uint32Bytes(static_cast<quint32>(size.height()));
}

QByteArray expectedI420(const QSize &size)
{
    const QSize chroma = chromaSize(size);
    return header(FrameFormat::I420, size) + planeBytes(0, size, size.width())
            + planeBytes(1, chroma, chroma.width()) + planeBytes(2, chroma, chroma.width());
}

// I420 frame memory with freely chosen strides, which a plain QVideoFrame does not allow
class I420Buffer : public QAbstractVideoBuffer
{
public:
    I420Buffer(const QSize &size, int lumaStride, int chromaStride)
        : m_format(size, QVideoFrameFormat::Format_YUV420P),
          m_strides{ lumaStride, chromaStride, chromaStride },
          m_planes{ planeBytes(0, size, lumaStride), planeBytes(1, chromaSize(size), chromaStride),
                    planeBytes(2, chromaSize(size), chromaStride) }
    {
    }

    MapData map(QVideoFrame::MapMode) override
    {
        MapData data;
        data.planeCount = 3;
        for (int plane = 0; plane < 3; ++plane) {
            data.bytesPerLine[plane] = m_strides[plane];
            data.data[plane] = reinterpret_cast<uchar *>(m_planes[plane].data());
            data.dataSize[plane] = static_cast<int>(m_planes[plane].size());
        }
        return data;
    }

    QVideoFrameFormat format() const override { return m_format; }

private:
    QVideoFrameFormat m_format;
    std::array<int, 3> m_strides;
    std::array<QByteArray, 3> m_planes;
};

} // namespace

void CameraFrameSerializerTest::serializesI420WithoutPadding_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("lumaStride");
    QTest::addColumn<int>("chromaStride");

    QTest::newRow("even size, tight") << QSize(4, 2) << 4 << 2;
    QTest::newRow("even size, padded") << QSize(4, 2) << 16 << 8;
    QTest::newRow("odd size, padded") << QSize(5, 3) << 8 << 4;
}

void CameraFrameSerializerTest::serializesI420WithoutPadding()
{
    QFETCH(QSize, size);
    QFETCH(int, lumaStride);
    QFETCH(int, chromaStride);

    const QVideoFrame frame(std::make_unique<I420Buffer>(size, lumaStride, chromaStride));
    const QByteArray serialized = CameraFrameSerializer::serialize(frame);

    QCOMPARE(CameraFrameSerializer::HeaderSize, static_cast<qsizetype>(12));
    QCOMPARE(serialized.left(CameraFrameSerializer::HeaderSize), header(FrameFormat::I420, size));
    QCOMPARE(serialized, expectedI420(size));
}

void CameraFrameSerializerTest::serializesOtherFormatsAsRgbx()
{
    const QSize size(3, 2);
    QImage image(size, QImage::Format_RGB32);
    QByteArray expected = header(FrameFormat::RGBX, size);

    for (int y = 0; y < size.height(); ++y) {
        for (int x = 0; x < size.width(); ++x) {
            const QRgb color = qRgb(50 + 10 * x, 100 + y, 200);
            image.setPixel(x, y, color);

            expected += static_cast<char>(qRed(color));
            expected += static_cast<char>(qGreen(color));
            expected += static_cast<char>(qBlue(color));
            expected += static_cast<char>(0xff);
        }
    }

    const QVideoFrame frame(image);
    QVERIFY(frame.pixelFormat() != QVideoFrameFormat::Format_YUV420P);
    QCOMPARE(CameraFrameSerializer::serialize(frame), expected);
}

void CameraFrameSerializerTest::rejectsInvalidFrame()
{
    QVERIFY(CameraFrameSerializer::serialize(QVideoFrame()).isEmpty());
}

QTEST_GUILESS_MAIN(CameraFrameSerializerTest)
