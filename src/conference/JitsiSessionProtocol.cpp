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

    if (type == "joined"_L1) {
        const QString participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"Joined without participant id"_s);
        }
        return Joined{ .participantId = participantId,
                       .role = roleFromString(object.value("role"_L1).toString()) };
    }

    if (type == "roleChanged"_L1) {
        const auto participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"Role change without participant id"_s);
        }
        return RoleChanged{ .participantId = participantId,
                            .role = roleFromString(object.value("role"_L1).toString()) };
    }

    if (type == "userJoined"_L1) {
        const auto participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"User joined without participant id"_s);
        }
        return UserJoined{ .participantId = participantId,
                           .displayName = object.value("displayName"_L1).toString(),
                           .role = roleFromString(object.value("role"_L1).toString()) };
    }

    if (type == "userLeft"_L1) {
        const auto participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"User left without participant id"_s);
        }
        return UserLeft{ .participantId = participantId };
    }

    if (type == "displayNameChanged"_L1) {
        const auto participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"Display name change without participant id"_s);
        }
        return DisplayNameChanged{ .participantId = participantId,
                                   .displayName = object.value("displayName"_L1).toString() };
    }

    if (type == "participantMuted"_L1) {
        const auto participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"Participant muted without participant id"_s);
        }

        const auto isAudioMuted = object.value("isAudioMuted"_L1);
        if (!isAudioMuted.isBool()) {
            return std::unexpected(u"Participant muted without isAudioMuted flag"_s);
        }

        const auto isVideoMuted = object.value("isVideoMuted"_L1);
        if (!isVideoMuted.isBool()) {
            return std::unexpected(u"Participant muted without isVideoMuted flag"_s);
        }

        return ParticipantMuted{ .participantId = participantId,
                                 .isAudioMuted = isAudioMuted.toBool(),
                                 .isVideoMuted = isVideoMuted.toBool() };
    }

    if (type == "handRaised"_L1) {
        const auto participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"Hand raised without participant id"_s);
        }

        const auto timestamp = object.value("timestamp"_L1);
        if (!timestamp.isDouble()) {
            return std::unexpected(u"Hand raised with invalid timestamp"_s);
        }

        const qint64 msecs = timestamp.toInteger();
        return HandRaised{ .participantId = participantId,
                           .dateTime = msecs > 0 ? QDateTime::fromMSecsSinceEpoch(msecs)
                                                 : QDateTime() };
    }

    if (type == "dominantSpeaker"_L1) {
        const auto participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"Dominant speaker without participant id"_s);
        }

        return DominantSpeaker{ .participantId = participantId };
    }

    if (type == "messageReceived"_L1) {
        const auto participantId = object.value("participantId"_L1).toString();
        if (participantId.isEmpty()) {
            return std::unexpected(u"Message without participant id"_s);
        }

        const auto text = object.value("text"_L1).toString();
        if (text.isEmpty()) {
            return std::unexpected(u"Message without text"_s);
        }

        const auto timestamp = object.value("timestamp"_L1);
        if (!timestamp.isDouble()) {
            return std::unexpected(u"Message with invalid timestamp"_s);
        }

        const auto isFromHistory = object.value("isFromHistory"_L1);
        if (!isFromHistory.isBool()) {
            return std::unexpected(u"Message without isFromHistory flag"_s);
        }

        return MessageReceived{ .participantId = participantId,
                                .displayName = object.value("displayName"_L1).toString(),
                                .text = text,
                                .dateTime = QDateTime::fromMSecsSinceEpoch(timestamp.toInteger()),
                                .isFromHistory = isFromHistory.toBool() };
    }

    if (type == "audioMuted"_L1) {
        const auto isMuted = object.value("isMuted"_L1);
        if (!isMuted.isBool()) {
            return std::unexpected(u"Audio mute state without isMuted"_s);
        }
        return AudioMuted{ .isMuted = isMuted.toBool() };
    }

    if (type == "videoMuted"_L1) {
        const auto isMuted = object.value("isMuted"_L1);
        if (!isMuted.isBool()) {
            return std::unexpected(u"Video mute state without isMuted"_s);
        }
        return VideoMuted{ .isMuted = isMuted.toBool() };
    }

    if (type == "devices"_L1) {
        return Devices{ .audioInputs = parseMediaDevices(object.value("audioInputs"_L1).toArray()),
                        .audioOutputs =
                                parseMediaDevices(object.value("audioOutputs"_L1).toArray()) };
    }

    if (type == "failed"_L1) {
        return Failed{ .reason = object.value("reason"_L1).toString() };
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
