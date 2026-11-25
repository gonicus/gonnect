#include "JitsiSessionProtocol.test.h"
#include "JitsiSessionProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

using Role = ConferenceUser::Role;

namespace {

// QVERIFY2 always evaluates message but error() is only valid w/o value
template <typename T>
QByteArray errorText(const std::expected<T, QString> &result)
{
    return result ? QByteArray() : result.error().toUtf8();
}
} // namespace

void JitsiSessionProtocolTest::parsesJoined_data()
{
    QTest::addColumn<QString>("roleString");
    QTest::addColumn<Role>("role");

    QTest::newRow("moderator") << u"moderator"_s << Role::Moderator;
    QTest::newRow("participant") << u"participant"_s << Role::User;
    QTest::newRow("none") << u"none"_s << Role::None;
    QTest::newRow("unknown") << u"visitor"_s << Role::None;
}

void JitsiSessionProtocolTest::parsesJoined()
{
    QFETCH(QString, roleString);
    QFETCH(Role, role);

    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"joined","participantId":"abcd1234","role":"%1"})"_s.arg(roleString));

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *joined = std::get_if<JitsiSessionProtocol::Joined>(&*event);
    QVERIFY(joined);
    QCOMPARE(joined->participantId, u"abcd1234"_s);
    QCOMPARE(joined->role, role);
}

void JitsiSessionProtocolTest::parsesRoleChanged()
{
    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"roleChanged","participantId":"abcd1234","role":"moderator"})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *roleChanged = std::get_if<JitsiSessionProtocol::RoleChanged>(&*event);
    QVERIFY(roleChanged);
    QCOMPARE(roleChanged->participantId, u"abcd1234"_s);
    QCOMPARE(roleChanged->role, Role::Moderator);
}

void JitsiSessionProtocolTest::parsesUserJoined()
{
    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"userJoined","participantId":"abcd1234","displayName":"Alice","role":"participant"})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *userJoined = std::get_if<JitsiSessionProtocol::UserJoined>(&*event);
    QVERIFY(userJoined);
    QCOMPARE(userJoined->participantId, u"abcd1234"_s);
    QCOMPARE(userJoined->displayName, u"Alice"_s);
    QCOMPARE(userJoined->role, Role::User);
}

void JitsiSessionProtocolTest::parsesUserLeft()
{
    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"userLeft","participantId":"abcd1234"})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *userLeft = std::get_if<JitsiSessionProtocol::UserLeft>(&*event);
    QVERIFY(userLeft);
    QCOMPARE(userLeft->participantId, u"abcd1234"_s);
}

void JitsiSessionProtocolTest::parsesDisplayNameChanged()
{
    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"displayNameChanged","participantId":"abcd1234","displayName":"Bob"})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *changed = std::get_if<JitsiSessionProtocol::DisplayNameChanged>(&*event);
    QVERIFY(changed);
    QCOMPARE(changed->participantId, u"abcd1234"_s);
    QCOMPARE(changed->displayName, u"Bob"_s);
}

void JitsiSessionProtocolTest::parsesParticipantMuted()
{
    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"participantMuted","participantId":"abcd1234","isAudioMuted":true,"isVideoMuted":false})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *muted = std::get_if<JitsiSessionProtocol::ParticipantMuted>(&*event);
    QVERIFY(muted);
    QCOMPARE(muted->participantId, u"abcd1234"_s);
    QVERIFY(muted->isAudioMuted);
    QVERIFY(!muted->isVideoMuted);
}

void JitsiSessionProtocolTest::parsesHandRaised_data()
{
    QTest::addColumn<qint64>("timestamp");
    QTest::addColumn<QDateTime>("dateTime");

    QTest::newRow("raised") << Q_INT64_C(1795969348296)
                            << QDateTime::fromMSecsSinceEpoch(1795969348296);
    QTest::newRow("lowered") << Q_INT64_C(0) << QDateTime();
}

void JitsiSessionProtocolTest::parsesHandRaised()
{
    QFETCH(qint64, timestamp);
    QFETCH(QDateTime, dateTime);

    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"handRaised","participantId":"abcd1234","timestamp":%1})"_s.arg(timestamp));

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *handRaised = std::get_if<JitsiSessionProtocol::HandRaised>(&*event);
    QVERIFY(handRaised);
    QCOMPARE(handRaised->participantId, u"abcd1234"_s);
    QCOMPARE(handRaised->dateTime, dateTime);
}

void JitsiSessionProtocolTest::parsesDominantSpeaker()
{
    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"dominantSpeaker","participantId":"abcd1234"})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *speaker = std::get_if<JitsiSessionProtocol::DominantSpeaker>(&*event);
    QVERIFY(speaker);
    QCOMPARE(speaker->participantId, u"abcd1234"_s);
}

void JitsiSessionProtocolTest::parseMessageReceived_data()
{
    QTest::addColumn<bool>("isFromHistory");

    QTest::newRow("live") << false;
    QTest::newRow("history") << true;
}

void JitsiSessionProtocolTest::parseMessageReceived()
{
    QFETCH(bool, isFromHistory);

    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"messageReceived","participantId":"abcd1234","displayName":"Alice",)"
            uR"("text":"Grüße 👋","timestamp":1795969348296,"isFromHistory":%1})"_s.arg(
                    isFromHistory ? "true"_L1 : "false"_L1));

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *message = std::get_if<JitsiSessionProtocol::MessageReceived>(&*event);
    QVERIFY(message);
    QCOMPARE(message->participantId, u"abcd1234"_s);
    QCOMPARE(message->displayName, u"Alice"_s);
    QCOMPARE(message->text, u"Grüße 👋"_s);
    QCOMPARE(message->dateTime, QDateTime::fromMSecsSinceEpoch(1795969348296));
    QCOMPARE(message->isFromHistory, isFromHistory);
}

void JitsiSessionProtocolTest::parsesAudioMuted()
{
    const auto event =
            JitsiSessionProtocol::parseEvent(uR"({"type":"audioMuted","isMuted":true})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *audioMuted = std::get_if<JitsiSessionProtocol::AudioMuted>(&*event);
    QVERIFY(audioMuted);
    QVERIFY(audioMuted->isMuted);
}

void JitsiSessionProtocolTest::parsesVideoMuted()
{
    const auto event =
            JitsiSessionProtocol::parseEvent(uR"({"type":"videoMuted","isMuted":false})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *videoMuted = std::get_if<JitsiSessionProtocol::VideoMuted>(&*event);
    QVERIFY(videoMuted);
    QVERIFY(!videoMuted->isMuted);
}

void JitsiSessionProtocolTest::parsesDevices()
{
    const auto event = JitsiSessionProtocol::parseEvent(uR"({"type":"devices",
        "audioInputs":[{"deviceId":"default","groupId":"g1","label":"Headset"},
                       {"deviceId":"","groupId":"g2","label":"No id"}],
        "audioOutputs":[{"deviceId":"speaker","groupId":"g3","label":"Speaker"}]})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *devices = std::get_if<JitsiSessionProtocol::Devices>(&*event);
    QVERIFY(devices);

    // Device without id cannot be selected
    QCOMPARE(devices->audioInputs.size(), 1);
    QCOMPARE(devices->audioInputs.first().deviceId, u"default"_s);
    QCOMPARE(devices->audioInputs.first().label, u"Headset"_s);
    QCOMPARE(devices->audioOutputs.size(), 1);
    QCOMPARE(devices->audioOutputs.first().groupId, u"g3"_s);
}

void JitsiSessionProtocolTest::parsesFailed()
{
    const auto event = JitsiSessionProtocol::parseEvent(
            uR"({"type":"failed","reason":"conference.passwordRequired"})"_s);

    QVERIFY2(event.has_value(), errorText(event).constData());
    const auto *failed = std::get_if<JitsiSessionProtocol::Failed>(&*event);
    QVERIFY(failed);
    QCOMPARE(failed->reason, u"conference.passwordRequired"_s);
}

void JitsiSessionProtocolTest::rejectsInvalidEvent_data()
{
    QTest::addColumn<QString>("message");

    QTest::newRow("no JSON") << u"joined"_s;
    QTest::newRow("unknown type") << uR"({"type":"dance"})"_s;
    QTest::newRow("joined without id") << uR"({"type":"joined","role":"moderator"})"_s;
    QTest::newRow("role change without id") << uR"({"type":"roleChanged","role":"moderator"})"_s;
    QTest::newRow("user join without id") << uR"({"type":"userJoined","displayName":"Alice"})"_s;
    QTest::newRow("user leave without id") << uR"({"type":"userLeft"})"_s;
    QTest::newRow("name change without id")
            << uR"({"type":"displayNameChanged","displayName":"Bob"})"_s;
    QTest::newRow("participant mute incomplete")
            << uR"({"type":"participantMuted","participantId":"abcd","isAudioMuted":true})"_s;
    QTest::newRow("hand without timestamp") << uR"({"type":"handRaised","participantId":"abcd"})"_s;
    QTest::newRow("speaker without id") << uR"({"type":"dominantSpeaker"})"_s;
    QTest::newRow("message without id")
            << uR"({"type":"messageReceived","text":"Hi","timestamp":1,"isFromHistory":false})"_s;
    QTest::newRow("message without text")
            << uR"({"type":"messageReceived","participantId":"abcd","timestamp":1,"isFromHistory":false})"_s;
    QTest::newRow("message without timestamp")
            << uR"({"type":"messageReceived","participantId":"abcd","text":"Hi","isFromHistory":false})"_s;
    QTest::newRow("audio mute without state") << uR"({"type":"audioMuted"})"_s;
    QTest::newRow("message without history flag")
            << uR"({"type":"messageReceived","participantId":"abcd","text":"Hi","timestamp":1})"_s;
}
void JitsiSessionProtocolTest::rejectsInvalidEvent()
{
    QFETCH(QString, message);

    QVERIFY(!JitsiSessionProtocol::parseEvent(message).has_value());
}

void JitsiSessionProtocolTest::createsLeaveCommand()
{
    const QJsonObject object =
            QJsonDocument::fromJson(JitsiSessionProtocol::leaveCommand().toUtf8()).object();

    QCOMPARE(object.value("type"_L1).toString(), u"leave"_s);
}

void JitsiSessionProtocolTest::createsAudioCommand()
{
    const auto toObject = [](const QString &message) {
        return QJsonDocument::fromJson(message.toUtf8()).object();
    };

    const QJsonObject mute = toObject(JitsiSessionProtocol::setAudioMutedCommand(true));
    QCOMPARE(mute.value("type"_L1).toString(), u"setAudioMuted"_s);
    QCOMPARE(mute.value("isMuted"_L1).toBool(), true);

    const QJsonObject input = toObject(JitsiSessionProtocol::setAudioInputDeviceCommand(u"mic"_s));
    QCOMPARE(input.value("type"_L1).toString(), u"setAudioInputDevice"_s);
    QCOMPARE(input.value("deviceId"_L1).toString(), u"mic"_s);

    const QJsonObject output =
            toObject(JitsiSessionProtocol::setAudioOutputDeviceCommand(u"speaker"_s));
    QCOMPARE(output.value("type"_L1).toString(), u"setAudioOutputDevice"_s);
    QCOMPARE(output.value("deviceId"_L1).toString(), u"speaker"_s);
}

void JitsiSessionProtocolTest::createsVideoCommand()
{
    const QJsonObject object =
            QJsonDocument::fromJson(JitsiSessionProtocol::setVideoMutedCommand(false).toUtf8())
                    .object();

    QCOMPARE(object.value("type"_L1).toString(), u"setVideoMuted"_s);
    QCOMPARE(object.value("isMuted"_L1).toBool(), false);
}

void JitsiSessionProtocolTest::createsModerationCommands()
{
    const auto toObject = [](const QString &message) {
        return QJsonDocument::fromJson(message.toUtf8()).object();
    };

    const QJsonObject kick = toObject(JitsiSessionProtocol::kickParticipantCommand(u"abcd"_s));
    QCOMPARE(kick.value("type"_L1).toString(), u"kickParticipant"_s);
    QCOMPARE(kick.value("participantId"_L1).toString(), u"abcd"_s);

    const QJsonObject grant = toObject(JitsiSessionProtocol::grantModeratorCommand(u"abcd"_s));
    QCOMPARE(grant.value("type"_L1).toString(), u"grantModerator"_s);
    QCOMPARE(grant.value("participantId"_L1).toString(), u"abcd"_s);

    const QJsonObject muteAll = toObject(JitsiSessionProtocol::muteAllCommand());
    QCOMPARE(muteAll.value("type"_L1).toString(), u"muteAll"_s);
}

void JitsiSessionProtocolTest::createsHandCommand()
{
    const QJsonObject object =
            QJsonDocument::fromJson(JitsiSessionProtocol::setHandRaisedCommand(true).toUtf8())
                    .object();

    QCOMPARE(object.value("type"_L1).toString(), u"setHandRaised"_s);
    QCOMPARE(object.value("isRaised"_L1).toBool(), true);
}

void JitsiSessionProtocolTest::createsMessageCommand()
{
    const QJsonObject object =
            QJsonDocument::fromJson(
                    JitsiSessionProtocol::sendMessageCommand(u"Grüße 👋"_s).toUtf8())
                    .object();

    QCOMPARE(object.value("type"_L1).toString(), u"sendMessage"_s);
    QCOMPARE(object.value("text"_L1).toString(), u"Grüße 👋"_s);
}

QTEST_GUILESS_MAIN(JitsiSessionProtocolTest)
