#include "JitsiSessionProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::Literals::StringLiterals;

namespace {

ConferenceUser::Role roleFromString(const QString &role)
{
    if (role == "moderator"_L1) {
        return ConferenceUser::Role::Moderator;
    }
    if (role == "participant"_L1) {
        return ConferenceUser::Role::User;
    }
    return ConferenceUser::Role::None;
}

QString commandMessage(QLatin1StringView type, QJsonObject object = {})
{
    object.insert("type"_L1, type);
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QList<JitsiSessionProtocol::MediaDevice> parseMediaDevices(const QJsonArray &entries)
{
    QList<JitsiSessionProtocol::MediaDevice> devices;
    for (const auto &entry : entries) {
        const auto object = entry.toObject();
        const JitsiSessionProtocol::MediaDevice device{
            .deviceId = object.value("deviceId"_L1).toString(),
            .groupId = object.value("groupId"_L1).toString(),
            .label = object.value("label"_L1).toString()
        };

        if (!device.deviceId.isEmpty()) {
            devices.append(device);
        }
    }
    return devices;
}

} // namespace

namespace JitsiSessionProtocol {
namespace {

// Reads fields of an event. On failure, it does not abort but delivers default values, remembering
// the first invalid field name in invalidField().
class EventFields
{
public:
    explicit EventFields(const QJsonObject &object) : m_object{ object } { }

    QString requiredString(QLatin1StringView key)
    {
        const auto value = m_object.value(key).toString();
        if (value.isEmpty()) {
            reject(key);
        }
        return value;
    }

    QString optionalString(QLatin1StringView key) const { return m_object.value(key).toString(); }

    bool requiredBool(QLatin1StringView key)
    {
        const auto value = m_object.value(key);
        if (!value.isBool()) {
            reject(key);
        }
        return value.toBool();
    }

    qint64 requiredInteger(QLatin1StringView key)
    {
        const auto value = m_object.value(key);
        if (!value.isDouble()) {
            reject(key);
        }
        return value.toInteger();
    }

    QJsonArray optionalArray(QLatin1StringView key) const { return m_object.value(key).toArray(); }

    /// Empty if all required fields are valid
    const QString &invalidField() const { return m_invalidField; }

private:
    void reject(QLatin1StringView key)
    {
        if (m_invalidField.isEmpty()) {
            m_invalidField = key;
        }
    }

    const QJsonObject &m_object;
    QString m_invalidField;
};

Event parseJoined(EventFields &fields)
{
    return Joined{ .participantId = fields.requiredString("participantId"_L1),
                   .role = roleFromString(fields.optionalString("role"_L1)) };
}

Event parseRoleChanged(EventFields &fields)
{
    return RoleChanged{ .participantId = fields.requiredString("participantId"_L1),
                        .role = roleFromString(fields.optionalString("role"_L1)) };
}

Event parseUserJoined(EventFields &fields)
{
    return UserJoined{ .participantId = fields.requiredString("participantId"_L1),
                       .displayName = fields.optionalString("displayName"_L1),
                       .role = roleFromString(fields.optionalString("role"_L1)) };
}

Event parseUserLeft(EventFields &fields)
{
    return UserLeft{ .participantId = fields.requiredString("participantId"_L1) };
}

Event parseDisplayNameChanged(EventFields &fields)
{
    return DisplayNameChanged{ .participantId = fields.requiredString("participantId"_L1),
                               .displayName = fields.optionalString("displayName"_L1) };
}

Event parseParticipantMuted(EventFields &fields)
{
    return ParticipantMuted{ .participantId = fields.requiredString("participantId"_L1),
                             .isAudioMuted = fields.requiredBool("isAudioMuted"_L1),
                             .isVideoMuted = fields.requiredBool("isVideoMuted"_L1) };
}

Event parseHandRaised(EventFields &fields)
{
    const QString participantId = fields.requiredString("participantId"_L1);
    const qint64 msecs = fields.requiredInteger("timestamp"_L1);

    // Epoch itself would be a valid date time
    return HandRaised{ .participantId = participantId,
                       .dateTime =
                               msecs > 0 ? QDateTime::fromMSecsSinceEpoch(msecs) : QDateTime() };
}

Event parseDominantSpeaker(EventFields &fields)
{
    return DominantSpeaker{ .participantId = fields.requiredString("participantId"_L1) };
}

Event parseMessageReceived(EventFields &fields)
{
    return MessageReceived{ .participantId = fields.requiredString("participantId"_L1),
                            .displayName = fields.optionalString("displayName"_L1),
                            .text = fields.requiredString("text"_L1),
                            .dateTime = QDateTime::fromMSecsSinceEpoch(
                                    fields.requiredInteger("timestamp"_L1)),
                            .isFromHistory = fields.requiredBool("isFromHistory"_L1) };
}

Event parseAudioMuted(EventFields &fields)
{
    return AudioMuted{ .isMuted = fields.requiredBool("isMuted"_L1) };
}

Event parseVideoMuted(EventFields &fields)
{
    return VideoMuted{ .isMuted = fields.requiredBool("isMuted"_L1) };
}

Event parseDevices(EventFields &fields)
{
    return Devices{ .audioInputs = parseMediaDevices(fields.optionalArray("audioInputs"_L1)),
                    .audioOutputs = parseMediaDevices(fields.optionalArray("audioOutputs"_L1)) };
}

Event parsePasswordRequired(EventFields &)
{
    return PasswordRequired{};
}

Event parseRoomLockChanged(EventFields &fields)
{
    return RoomLockChanged{ .isLocked = fields.requiredBool("isLocked"_L1) };
}

Event parseLobbyWaiting(EventFields &)
{
    return LobbyWaiting{};
}

Event parseLobbyChanged(EventFields &fields)
{
    return LobbyChanged{ .isEnabled = fields.requiredBool("isEnabled"_L1) };
}

Event parseLobbyUserJoined(EventFields &fields)
{
    return LobbyUserJoined{ .participantId = fields.requiredString("participantId"_L1),
                            .displayName = fields.optionalString("displayName"_L1) };
}

Event parseLobbyUserLeft(EventFields &fields)
{
    return LobbyUserLeft{ .participantId = fields.requiredString("participantId"_L1) };
}

Event parseFailed(EventFields &fields)
{
    return Failed{ .reason = fields.optionalString("reason"_L1) };
}

struct EventParser
{
    QLatin1StringView type;
    Event (*parse)(EventFields &fields);
};

constexpr EventParser eventParsers[] = {
    { "joined"_L1, parseJoined },
    { "roleChanged"_L1, parseRoleChanged },
    { "userJoined"_L1, parseUserJoined },
    { "userLeft"_L1, parseUserLeft },
    { "displayNameChanged"_L1, parseDisplayNameChanged },
    { "participantMuted"_L1, parseParticipantMuted },
    { "handRaised"_L1, parseHandRaised },
    { "dominantSpeaker"_L1, parseDominantSpeaker },
    { "messageReceived"_L1, parseMessageReceived },
    { "audioMuted"_L1, parseAudioMuted },
    { "videoMuted"_L1, parseVideoMuted },
    { "devices"_L1, parseDevices },
    { "passwordRequired"_L1, parsePasswordRequired },
    { "roomLockChanged"_L1, parseRoomLockChanged },
    { "lobbyWaiting"_L1, parseLobbyWaiting },
    { "lobbyChanged"_L1, parseLobbyChanged },
    { "lobbyUserJoined"_L1, parseLobbyUserJoined },
    { "lobbyUserLeft"_L1, parseLobbyUserLeft },
    { "failed"_L1, parseFailed },
};

} // namespace

QString joinWithPasswordCommand(const QString &password)
{
    return commandMessage("joinWithPassword"_L1, { { "password"_L1, password } });
}

QString setRoomPasswordCommand(const QString &password)
{
    return commandMessage("setRoomPassword"_L1, { { "password"_L1, password } });
}

QString setLobbyEnabledCommand(bool isEnabled)
{
    return commandMessage("setLobbyEnabled"_L1, { { "isEnabled"_L1, isEnabled } });
}

QString answerLobbyUserCommand(const QString &participantId, bool isApproved)
{

    return commandMessage(
            "answerLobbyUser"_L1,
            { { "participantId"_L1, participantId }, { "isApproved"_L1, isApproved } });
}

// namespace
} // namespace JitsiSessionProtocol

std::expected<JitsiSessionProtocol::Event, QString>
JitsiSessionProtocol::parseEvent(const QString &message)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (!document.isObject()) {
        return std::unexpected(QStringLiteral("Invalid JSON: %1").arg(error.errorString()));
    }

    const auto object = document.object();
    const QString type = object.value("type"_L1).toString();

    for (const auto &eventParser : eventParsers) {
        if (type == eventParser.type) {
            EventFields fields(object);
            const Event event = eventParser.parse(fields);

            if (!fields.invalidField().isEmpty()) {
                return std::unexpected(
                        QStringLiteral("Event '%1' has missing or invalid field '%2'")
                                .arg(type, fields.invalidField()));
            }
            return event;
        }
    }

    return std::unexpected(QStringLiteral("Unknown event type '%1'").arg(type));
}

QString JitsiSessionProtocol::leaveCommand()
{
    return commandMessage("leave"_L1);
}

QString JitsiSessionProtocol::setAudioMutedCommand(bool isMuted)
{
    return commandMessage("setAudioMuted"_L1, { { "isMuted"_L1, isMuted } });
}

QString JitsiSessionProtocol::setVideoMutedCommand(bool isMuted)
{
    return commandMessage("setVideoMuted"_L1, { { "isMuted"_L1, isMuted } });
}

QString JitsiSessionProtocol::setAudioInputDeviceCommand(const QString &deviceId)
{
    return commandMessage("setAudioInputDevice"_L1, { { "deviceId"_L1, deviceId } });
}

QString JitsiSessionProtocol::setAudioOutputDeviceCommand(const QString &deviceId)
{
    return commandMessage("setAudioOutputDevice"_L1, { { "deviceId"_L1, deviceId } });
}

QString JitsiSessionProtocol::kickParticipantCommand(const QString &participantId)
{
    return commandMessage("kickParticipant"_L1, { { "participantId"_L1, participantId } });
}

QString JitsiSessionProtocol::grantModeratorCommand(const QString &participantId)
{
    return commandMessage("grantModerator"_L1, { { "participantId"_L1, participantId } });
}

QString JitsiSessionProtocol::muteAllCommand()
{
    return commandMessage("muteAll"_L1);
}

QString JitsiSessionProtocol::setHandRaisedCommand(bool isRaised)
{
    return commandMessage("setHandRaised"_L1, { { "isRaised"_L1, isRaised } });
}

QString JitsiSessionProtocol::sendMessageCommand(const QString &text)
{

    return commandMessage("sendMessage"_L1, { { "text"_L1, text } });
}
