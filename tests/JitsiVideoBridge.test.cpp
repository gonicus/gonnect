#include "JitsiVideoBridge.test.h"
#include "IvfReader.h"
#include "JitsiVideoProtocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QVideoFrame>
#include <QVideoSink>
#include <QWebSocket>

using namespace Qt::Literals::StringLiterals;

namespace {

const QUrl JitsiOrigin(u"https://meet.example.org"_s);
const QString TrackId = u"remote-video-1"_s;
const QString SourceName = u"fd2257db-v0"_s;
constexpr quint8 Vp8PayloadType = 96;

const QString TracksMessage = uR"({"type":"tracks","tracks":[{"trackId":"remote-video-1",
    "sourceName":"fd2257db-v0","participantId":"fd2257db","videoType":"camera",
    "isMuted":false}]})"_s;
const QString CodecsMessage = uR"({"type":"codecs","codecs":{"96":"video/VP8"}})"_s;

QUrl connectionUrl(const JitsiVideoBridge &bridge, const QString &pathAndQuery)
{
    return QUrl(bridge.endpoint().toString() + pathAndQuery);
}

QUrl controlUrl(const JitsiVideoBridge &bridge)
{
    return connectionUrl(bridge, u"/control?token="_s + bridge.token());
}

QUrl trackUrl(const JitsiVideoBridge &bridge)
{
    return connectionUrl(bridge, u"/track?token=%1&id=%2"_s.arg(bridge.token(), TrackId));
}

QByteArray frameMessage(const QByteArray &payload, bool isKeyFrame)
{
    QByteArray message(JitsiVideoProtocol::FrameHeaderSize, '\0');
    auto *header = reinterpret_cast<uchar *>(message.data());
    header[0] = JitsiVideoProtocol::Version;
    header[1] = isKeyFrame ? JitsiVideoProtocol::KeyFrameFlag : 0;
    header[2] = Vp8PayloadType;
    message.append(payload);
    return message;
}

bool openAndWait(QWebSocket &socket, const QUrl &url)
{
    QSignalSpy connected(&socket, &QWebSocket::connected);
    socket.open(url);
    return connected.wait();
}

} // namespace

void JitsiVideoBridgeTest::init()
{
    m_model = std::make_unique<ConferenceVideoModel>();
    m_bridge = std::make_unique<JitsiVideoBridge>(m_model.get(), JitsiOrigin);
    QVERIFY(m_bridge->start().has_value());
}

void JitsiVideoBridgeTest::cleanup()
{
    m_bridge.reset();
    m_model.reset();
}

void JitsiVideoBridgeTest::rejectsInvalidToken()
{
    QWebSocket control(JitsiOrigin.toString());
    QSignalSpy disconnected(&control, &QWebSocket::disconnected);

    control.open(connectionUrl(*m_bridge, u"/control?token=wrong"_s));

    QTRY_VERIFY(!disconnected.isEmpty());
    QCOMPARE(control.closeCode(), QWebSocketProtocol::CloseCodePolicyViolated);
}

void JitsiVideoBridgeTest::rejectsForeignOrigin()
{
    QWebSocket control(u"https://attacker.example.org"_s);
    QSignalSpy connected(&control, &QWebSocket::connected);
    QSignalSpy disconnected(&control, &QWebSocket::disconnected);

    control.open(controlUrl(*m_bridge));

    QTRY_VERIFY(!disconnected.isEmpty());
    QVERIFY(connected.isEmpty());
}

void JitsiVideoBridgeTest::decodesTrackIntoSource()
{
    const auto frames = readIvfFrames(QStringLiteral(TEST_DATA_DIR "/video/vp8.ivf"));
    QVERIFY(frames.has_value());
    QVERIFY(!frames->isEmpty());

    QWebSocket control(JitsiOrigin.toString());
    QVERIFY(openAndWait(control, controlUrl(*m_bridge)));
    control.sendTextMessage(TracksMessage);
    QTRY_COMPARE(m_model->rowCount(), 1);

    ConferenceVideoSource *source = m_model->source(SourceName);
    QVERIFY(source);
    QVideoSink sink;
    source->framePump()->setSink(&sink);

    // The first key frame may arrive before the track is assigned
    QWebSocket track(JitsiOrigin.toString());
    connect(&track, &QWebSocket::textMessageReceived, &track,
            [&track, &frames]() { track.sendBinaryMessage(frameMessage(frames->first(), true)); });

    QVERIFY(openAndWait(track, trackUrl(*m_bridge)));
    track.sendTextMessage(CodecsMessage);
    for (qsizetype i = 0; i < frames->size(); ++i) {
        track.sendBinaryMessage(frameMessage(frames->at(i), i == 0));
    }

    QTRY_VERIFY(sink.videoFrame().isValid());
    QCOMPARE(sink.videoFrame().size(), QSize(64, 48));
}

void JitsiVideoBridgeTest::requestsKeyFrameForInterFrame()
{
    QWebSocket control(JitsiOrigin.toString());
    QVERIFY(openAndWait(control, controlUrl(*m_bridge)));
    control.sendTextMessage(TracksMessage);
    QTRY_COMPARE(m_model->rowCount(), 1);

    QWebSocket track(JitsiOrigin.toString());
    QSignalSpy requests(&track, &QWebSocket::textMessageReceived);
    QVERIFY(openAndWait(track, trackUrl(*m_bridge)));

    track.sendTextMessage(CodecsMessage);
    track.sendBinaryMessage(frameMessage("inter frame"_ba, false));

    QTRY_VERIFY(!requests.isEmpty());
    QCOMPARE(requests.first().first().toString(), JitsiVideoProtocol::keyFrameRequestMessage());
}

void JitsiVideoBridgeTest::sendsReceiverConstraints()
{
    QWebSocket control(JitsiOrigin.toString());
    QSignalSpy messages(&control, &QWebSocket::textMessageReceived);
    QVERIFY(openAndWait(control, controlUrl(*m_bridge)));
    control.sendTextMessage(TracksMessage);
    QTRY_COMPARE(m_model->rowCount(), 1);

    m_model->source(SourceName)->setRequestedHeight(500);

    // Constraints of the tile before may arrive first
    const auto lastMaxHeight = [&messages]() {
        if (messages.isEmpty()) {
            return -1;
        }
        const QJsonObject message =
                QJsonDocument::fromJson(messages.last().first().toString().toUtf8()).object();
        return message.value("constraints"_L1)
                .toObject()
                .value(SourceName)
                .toObject()
                .value("maxHeight"_L1)
                .toInt(-1);
    };
    QTRY_COMPARE(lastMaxHeight(), 360);
}

QTEST_GUILESS_MAIN(JitsiVideoBridgeTest)
