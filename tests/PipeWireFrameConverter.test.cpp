#include "PipeWireFrameConverter.test.h"
#include "PipeWireFrameConverter.h"

#include <QTest>
#include <QVideoFrame>

#include <cstdlib>
#include <functional>
#include <optional>

namespace {

constexpr QSize FrameSize(16, 4);
constexpr char PaddingByte = char(0xff);
constexpr uint8_t SourceU = 90;
constexpr uint8_t SourceV = 200;
constexpr int NeutralChroma = 128;

// Allows small rounding differences between FFmpeg versions
constexpr int Tolerance = 2;

// Holds a single-block PipeWire buffer as delivered by the V4L2 plugin
class TestBuffer
{
public:
    TestBuffer(const QByteArray &content, int32_t stride) : m_bytes(content)
    {
        m_chunk.size = uint32_t(m_bytes.size());
        m_chunk.stride = stride;

        m_data.type = SPA_DATA_MemPtr;
        m_data.maxsize = uint32_t(m_bytes.size());
        m_data.data = m_bytes.data();
        m_data.chunk = &m_chunk;

        m_buffer.n_datas = 1;
        m_buffer.datas = &m_data;
    }
    Q_DISABLE_COPY_MOVE(TestBuffer)

    const spa_buffer &buffer() const { return m_buffer; }
    spa_chunk &chunk() { return m_chunk; }

private:
    QByteArray m_bytes;
    spa_chunk m_chunk = { };
    spa_data m_data = { };
    spa_buffer m_buffer = { };
};

spa_video_info_raw rawInfo(spa_video_format format)
{
    spa_video_info_raw info = { };
    info.format = format;
    info.size = spa_rectangle{ uint32_t(FrameSize.width()), uint32_t(FrameSize.height()) };
    return info;
}

int lumaAt(int x, int y)
{
    return 16 + 8 * x + y;
}

QByteArray padded(QByteArray row, int stride)
{
    row.resize(stride, PaddingByte);
    return row;
}

QByteArray lumaPlane(int stride)
{
    QByteArray plane;
    for (int y = 0; y < FrameSize.height(); ++y) {
        QByteArray row;
        for (int x = 0; x < FrameSize.width(); ++x) {
            row += char(lumaAt(x, y));
        }
        plane += padded(row, stride);
    }
    return plane;
}

QByteArray makeI420(int stride)
{
    const int chromaWidth = FrameSize.width() / 2;
    const int chromaHeight = FrameSize.height() / 2;

    QByteArray bytes = lumaPlane(stride);
    for (const uint8_t value : { SourceU, SourceV }) {
        for (int y = 0; y < chromaHeight; ++y) {
            bytes += padded(QByteArray(chromaWidth, char(value)), stride / 2);
        }
    }
    return bytes;
}

QByteArray makeNv12(int stride)
{
    QByteArray chromaRow;
    for (int x = 0; x < FrameSize.width() / 2; ++x) {
        chromaRow += char(SourceU);
        chromaRow += char(SourceV);
    }

    QByteArray bytes = lumaPlane(stride);
    for (int y = 0; y < FrameSize.height() / 2; ++y) {
        bytes += padded(chromaRow, stride);
    }
    return bytes;
}

QByteArray makeYuy2(int stride)
{
    QByteArray bytes;
    for (int y = 0; y < FrameSize.height(); ++y) {
        QByteArray row;
        for (int x = 0; x < FrameSize.width(); x += 2) {
            row += char(lumaAt(x, y));
            row += char(SourceU);
            row += char(lumaAt(x + 1, y));
            row += char(SourceV);
        }
        bytes += padded(row, stride);
    }
    return bytes;
}

// Returns a description of the first pixel deviating from the expectation, empty if none
QString describeMismatch(const QVideoFrame &frame, int plane, const QSize &planeSize,
                         const std::function<int(int, int)> &expected)
{
    for (int y = 0; y < planeSize.height(); ++y) {
        for (int x = 0; x < planeSize.width(); ++x) {
            const int actual = frame.bits(plane)[y * frame.bytesPerLine(plane) + x];
            if (std::abs(actual - expected(x, y)) > Tolerance) {
                return QStringLiteral("Plane %1 at (%2, %3): %4 instead of %5")
                        .arg(plane)
                        .arg(x)
                        .arg(y)
                        .arg(actual)
                        .arg(expected(x, y));
            }
        }
    }
    return { };
}

} // namespace

void PipeWireFrameConverterTest::convertsYuvFormats_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<int>("stride");

    QTest::newRow("I420 without stride") << int(SPA_VIDEO_FORMAT_I420) << makeI420(16) << 0;
    QTest::newRow("I420") << int(SPA_VIDEO_FORMAT_I420) << makeI420(16) << 16;
    QTest::newRow("I420 padded") << int(SPA_VIDEO_FORMAT_I420) << makeI420(32) << 32;
    QTest::newRow("NV12 padded") << int(SPA_VIDEO_FORMAT_NV12) << makeNv12(32) << 32;
    QTest::newRow("YUY2 padded") << int(SPA_VIDEO_FORMAT_YUY2) << makeYuy2(48) << 48;
}

void PipeWireFrameConverterTest::convertsYuvFormats()
{
    QFETCH(int, format);
    QFETCH(QByteArray, source);
    QFETCH(int, stride);

    PipeWireFrameConverter converter;
    QVERIFY(converter.setFormat(rawInfo(spa_video_format(format))));

    const TestBuffer buffer(source, stride);
    const std::optional<QVideoFrame> converted = converter.convert(buffer.buffer());
    QVERIFY(converted);

    QVideoFrame frame = *converted;
    QCOMPARE(frame.pixelFormat(), QVideoFrameFormat::Format_YUV420P);
    QCOMPARE(frame.size(), FrameSize);
    QVERIFY(frame.map(QVideoFrame::ReadOnly));

    const QSize chromaSize = FrameSize / 2;
    QCOMPARE(describeMismatch(frame, 0, FrameSize, lumaAt), QString());
    QCOMPARE(describeMismatch(frame, 1, chromaSize, [](int, int) { return SourceU; }), QString());
    QCOMPARE(describeMismatch(frame, 2, chromaSize, [](int, int) { return SourceV; }), QString());
}

void PipeWireFrameConverterTest::convertsToLimitedRange_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("bytesPerPixel");
    QTest::addColumn<int>("sourceValue");
    QTest::addColumn<int>("expectedLuma");

    QTest::newRow("BGRx black") << int(SPA_VIDEO_FORMAT_BGRx) << 4 << 0 << 16;
    QTest::newRow("BGRx white") << int(SPA_VIDEO_FORMAT_BGRx) << 4 << 255 << 235;
    QTest::newRow("GRAY8 black") << int(SPA_VIDEO_FORMAT_GRAY8) << 1 << 0 << 16;
    QTest::newRow("GRAY8 white") << int(SPA_VIDEO_FORMAT_GRAY8) << 1 << 255 << 235;
}

void PipeWireFrameConverterTest::convertsToLimitedRange()
{
    QFETCH(int, format);
    QFETCH(int, bytesPerPixel);
    QFETCH(int, sourceValue);
    QFETCH(int, expectedLuma);

    PipeWireFrameConverter converter;
    QVERIFY(converter.setFormat(rawInfo(spa_video_format(format))));

    const int stride = FrameSize.width() * bytesPerPixel;
    const TestBuffer buffer(QByteArray(stride * FrameSize.height(), char(sourceValue)), stride);
    const std::optional<QVideoFrame> converted = converter.convert(buffer.buffer());
    QVERIFY(converted);

    QVideoFrame frame = *converted;
    QVERIFY(frame.map(QVideoFrame::ReadOnly));

    const QSize chromaSize = FrameSize / 2;
    const auto luma = [expectedLuma](int, int) { return expectedLuma; };
    const auto neutral = [](int, int) { return NeutralChroma; };
    QCOMPARE(describeMismatch(frame, 0, FrameSize, luma), QString());
    QCOMPARE(describeMismatch(frame, 1, chromaSize, neutral), QString());
    QCOMPARE(describeMismatch(frame, 2, chromaSize, neutral), QString());
}

void PipeWireFrameConverterTest::rejectsUnsupportedFormat()
{
    PipeWireFrameConverter converter;
    QVERIFY(!converter.setFormat(rawInfo(SPA_VIDEO_FORMAT_UYVY)));
    QVERIFY(!converter.isValid());
}

void PipeWireFrameConverterTest::rejectsBufferWithoutFormat()
{
    PipeWireFrameConverter converter;
    const TestBuffer buffer(makeI420(16), 16);
    QVERIFY(!converter.convert(buffer.buffer()));
}

void PipeWireFrameConverterTest::rejectsShortChunk()
{
    PipeWireFrameConverter converter;
    QVERIFY(converter.setFormat(rawInfo(SPA_VIDEO_FORMAT_I420)));

    TestBuffer buffer(makeI420(16), 16);
    buffer.chunk().size -= 1;
    QVERIFY(!converter.convert(buffer.buffer()));
}

void PipeWireFrameConverterTest::rejectsCorruptedChunk()
{
    PipeWireFrameConverter converter;
    QVERIFY(converter.setFormat(rawInfo(SPA_VIDEO_FORMAT_I420)));

    TestBuffer buffer(makeI420(16), 16);
    buffer.chunk().flags |= SPA_CHUNK_FLAG_CORRUPTED;
    QVERIFY(!converter.convert(buffer.buffer()));
}

QTEST_GUILESS_MAIN(PipeWireFrameConverterTest)
