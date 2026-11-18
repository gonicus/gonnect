#include "VideoDecoder.test.h"
#include "VideoDecoder.h"

#include <QFile>
#include <QTest>
#include <QVideoFrame>
#include <QtEndian>

#include <expected>
#include <optional>

Q_DECLARE_METATYPE(VideoDecoder::Codec)

namespace {

using Codec = VideoDecoder::Codec;

constexpr QSize FixtureSize(64, 48);

// BT.601 limited range red as rendered by "ffmpeg -f lavfi -i color=c=red"
constexpr int RedY = 81;
constexpr int RedU = 90;
constexpr int RedV = 240;

// Allows small coding losses
constexpr int Tolerance = 4;

constexpr qsizetype IvfFileHeaderSize = 32;
constexpr qsizetype IvfFrameHeaderSize = 12;

// Frame payloads of an IVF file, see tests/resources/video/README.md
std::optional<QList<QByteArray>> readIvfFrames(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    const QByteArray content = file.readAll();
    if (!content.startsWith("DKIF")) {
        return std::nullopt;
    }

    QList<QByteArray> frames;
    qsizetype offset = IvfFileHeaderSize;
    while (offset + IvfFrameHeaderSize <= content.size()) {
        const auto frameSize =
                static_cast<qsizetype>(qFromLittleEndian<quint32>(content.constData() + offset));
        offset += IvfFrameHeaderSize;
        if (offset + frameSize > content.size()) {
            return std::nullopt;
        }
        frames.append(content.sliced(offset, frameSize));
        offset += frameSize;
    }
    return frames;
}

// Value in the middle of a plane of a mapped I420 frame
int centerValue(const QVideoFrame &frame, int plane)
{
    const int divisor = plane == 0 ? 1 : 2;
    const int x = frame.width() / 2 / divisor;
    const int y = frame.height() / 2 / divisor;
    return frame.bits(plane)[y * frame.bytesPerLine(plane) + x];
}

template<typename T>
QString errorOf(const std::expected<T, QString> &result)
{
    return result.has_value() ? QString() : result.error();
}

} // namespace

void VideoDecoderTest::decodesIvf_data()
{
    QTest::addColumn<Codec>("codec");
    QTest::addColumn<QString>("fileName");

    QTest::newRow("VP8") << Codec::VP8 << QStringLiteral("vp8.ivf");
    QTest::newRow("VP9") << Codec::VP9 << QStringLiteral("vp9.ivf");
    QTest::newRow("VP9 10 bit") << Codec::VP9 << QStringLiteral("vp9-10bit.ivf");
    QTest::newRow("AV1") << Codec::AV1 << QStringLiteral("av1.ivf");
}

void VideoDecoderTest::decodesIvf()
{
    QFETCH(Codec, codec);
    QFETCH(QString, fileName);

    if (!VideoDecoder::isSupported(codec)) {
        QSKIP("Decoder not available in this FFmpeg build");
    }

    const auto encodedFrames = readIvfFrames(QStringLiteral(TEST_DATA_DIR "/video/") + fileName);
    QVERIFY(encodedFrames.has_value());
    QVERIFY(!encodedFrames->isEmpty());

    VideoDecoder decoder;
    const auto opened = decoder.open(codec);
    QVERIFY2(opened.has_value(), qPrintable(errorOf(opened)));

    qsizetype decodedFrames = 0;
    for (const QByteArray &payload : *encodedFrames) {
        const auto frames = decoder.decode(payload);
        QVERIFY2(frames.has_value(), qPrintable(errorOf(frames)));

        for (QVideoFrame frame : *frames) {
            QCOMPARE(frame.pixelFormat(), QVideoFrameFormat::Format_YUV420P);
            QCOMPARE(frame.size(), FixtureSize);

            QVERIFY(frame.map(QVideoFrame::ReadOnly));
            QCOMPARE_LE(qAbs(centerValue(frame, 0) - RedY), Tolerance);
            QCOMPARE_LE(qAbs(centerValue(frame, 1) - RedU), Tolerance);
            QCOMPARE_LE(qAbs(centerValue(frame, 2) - RedV), Tolerance);
            frame.unmap();
        }
        decodedFrames += frames->size();
    }

    // Fixtures are encoded without hidden frames
    QCOMPARE(decodedFrames, encodedFrames->size());
}

void VideoDecoderTest::rejectsInterFrameWithoutKeyFrame()
{
    VideoDecoder decoder;
    QVERIFY(decoder.open(Codec::VP8).has_value());

    // Cleared bit 0 in the first byte marks a VP8 inter frame
    const QByteArray interFrame(64, static_cast<char>(0x42));
    QVERIFY(!decoder.decode(interFrame).has_value());
}

void VideoDecoderTest::rejectsDecodeWhenNotOpen()
{
    VideoDecoder decoder;
    QVERIFY(!decoder.decode(QByteArray(16, '\0')).has_value());
}

QTEST_GUILESS_MAIN(VideoDecoderTest)
