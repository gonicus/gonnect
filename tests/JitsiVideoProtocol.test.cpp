#include "JitsiVideoProtocol.test.h"
#include "JitsiVideoProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QtEndian>

using namespace Qt::Literals::StringLiterals;

namespace {

using Type = ConferenceVideoSource::Type;

const JitsiVideoProtocol::CodecMap Codecs = { { 96, VideoCodec::VP8 }, { 98, VideoCodec::VP9 } };

QByteArray frameMessage(quint8 version, quint8 flags, quint8 payloadType, quint32 rtpTimestamp,
                        QSize size, QByteArrayView payload)
{
    QByteArray message(JitsiVideoProtocol::FrameHeaderSize, '\0');
    auto *header = reinterpret_cast<uchar *>(message.data());
    header[0] = version;
    header[1] = flags;
    header[2] = payloadType;
    qToLittleEndian<quint32>(rtpTimestamp, header + 4);
    qToLittleEndian<quint16>(static_cast<quint16>(size.width()), header + 8);
    qToLittleEndian<quint16>(static_cast<quint16>(size.height()), header + 10);
    message.append(payload);
    return message;
}

} // namespace

void JitsiVideoProtocolTest::parsesFrame()
{
    const QByteArray message =
            frameMessage(JitsiVideoProtocol::Version, JitsiVideoProtocol::KeyFrameFlag, 98,
                         0x12345678, QSize(640, 360), "payload");

    const auto frame = JitsiVideoProtocol::parseFrame(message, Codecs);

    QVERIFY(frame.has_value());
    QCOMPARE(frame->codec, VideoCodec::VP9);
    QVERIFY(frame->isKeyFrame);
    QCOMPARE(frame->rtpTimestamp, 0x12345678u);
    QCOMPARE(frame->size, QSize(640, 360));
    QCOMPARE(frame->payload, "payload"_ba);
}

void JitsiVideoProtocolTest::rejectsInvalidFrame_data()
{
    QTest::addColumn<QByteArray>("message");

    const QByteArray valid =
            frameMessage(JitsiVideoProtocol::Version, 0, 96, 0, QSize(), "payload");

    QTest::newRow("truncated") << valid.first(JitsiVideoProtocol::FrameHeaderSize - 1);
    QTest::newRow("old version") << frameMessage(1, 0, 96, 0, QSize(), "payload");
    QTest::newRow("unknown payload type")
            << frameMessage(JitsiVideoProtocol::Version, 0, 111, 0, QSize(), "payload");
}

void JitsiVideoProtocolTest::rejectsInvalidFrame()
{
    QFETCH(QByteArray, message);

    QVERIFY(!JitsiVideoProtocol::parseFrame(message, Codecs).has_value());
}

void JitsiVideoProtocolTest::parsesCodecs()
{
    const QString message = uR"({"type":"codecs","codecs":{
        "96":"video/VP8", "98":"video/vp9", "45":"video/AV1",
        "102":"video/H264", "x":"video/VP8", "200":"video/VP8"}})"_s;

    const auto codecs = JitsiVideoProtocol::parseCodecs(message);

    QVERIFY(codecs.has_value());
    const JitsiVideoProtocol::CodecMap expected = { { 96, VideoCodec::VP8 },
                                                    { 98, VideoCodec::VP9 },
                                                    { 45, VideoCodec::AV1 } };
    QCOMPARE(*codecs, expected);
}

void JitsiVideoProtocolTest::rejectsMessageOfOtherType()
{
    const QString tracksMessage = uR"({"type":"tracks","tracks":[]})"_s;

    QVERIFY(!JitsiVideoProtocol::parseCodecs(tracksMessage).has_value());
    QVERIFY(!JitsiVideoProtocol::parseTracks(u"no json"_s).has_value());
}

void JitsiVideoProtocolTest::parsesTracks()
{
    const QString message = uR"({"type":"tracks","tracks":[
        {"trackId":"remote-video-1","sourceName":"fd2257db-v0","participantId":"fd2257db",
         "videoType":"camera","isMuted":false},
        {"trackId":"remote-video-2","sourceName":"fd2257db-v1","participantId":"fd2257db",
         "videoType":"desktop","isMuted":true},
        {"sourceName":"fd2257db-v2","participantId":"fd2257db","videoType":"camera"}]})"_s;

    const auto tracks = JitsiVideoProtocol::parseTracks(message);

    QVERIFY(tracks.has_value());
    QCOMPARE(tracks->size(), 2);

    const auto &camera = tracks->at(0);
    QCOMPARE(camera.trackId, u"remote-video-1"_s);
    QCOMPARE(camera.sourceName, u"fd2257db-v0"_s);
    QCOMPARE(camera.participantId, u"fd2257db"_s);
    QCOMPARE(camera.type, Type::Camera);
    QVERIFY(!camera.isMuted);

    const auto &screen = tracks->at(1);
    QCOMPARE(screen.type, Type::ScreenShare);
    QVERIFY(screen.isMuted);
}

void JitsiVideoProtocolTest::createsKeyFrameRequest()
{
    const QJsonObject message =
            QJsonDocument::fromJson(JitsiVideoProtocol::keyFrameRequestMessage().toUtf8()).object();

    QCOMPARE(message.value("type"_L1).toString(), u"keyFrameRequest"_s);
}

void JitsiVideoProtocolTest::mapsQualityHeight_data()
{
    QTest::addColumn<int>("displayedHeight");
    QTest::addColumn<int>("expectedHeight");

    QTest::newRow("hidden") << 0 << 0;
    QTest::newRow("tiny") << 50 << 180;
    QTest::newRow("below standard") << 359 << 180;
    QTest::newRow("standard") << 360 << 360;
    QTest::newRow("below high") << 719 << 360;
    QTest::newRow("high") << 720 << 720;
    QTest::newRow("beyond full hd") << 1500 << 1080;
}

void JitsiVideoProtocolTest::mapsQualityHeight()
{
    QFETCH(int, displayedHeight);
    QFETCH(int, expectedHeight);

    QCOMPARE(JitsiVideoProtocol::qualityHeight(displayedHeight), expectedHeight);
}

void JitsiVideoProtocolTest::createsReceiverConstraints()
{
    const QString message = JitsiVideoProtocol::receiverConstraintsMessage(
            { { .sourceName = u"fd2257db-v0"_s, .maxHeight = 360 },
              { .sourceName = u"fd2257db-v1"_s,
                .maxHeight = JitsiVideoProtocol::MaxReceiverHeight,
                .isOnStage = true } });

    const QJsonObject object = QJsonDocument::fromJson(message.toUtf8()).object();
    QCOMPARE(object.value("type"_L1).toString(), u"receiverConstraints"_s);

    const QJsonObject constraints = object.value("constraints"_L1).toObject();
    QCOMPARE(constraints.value("fd2257db-v0"_L1).toObject().value("maxHeight"_L1).toInt(), 360);
    QCOMPARE(constraints.value("fd2257db-v1"_L1).toObject().value("maxHeight"_L1).toInt(),
             JitsiVideoProtocol::MaxReceiverHeight);
    QCOMPARE(object.value("onStageSources"_L1).toArray(), QJsonArray{ u"fd2257db-v1"_s });
}

QTEST_GUILESS_MAIN(JitsiVideoProtocolTest)
