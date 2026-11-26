#pragma once

#include "ConferenceUser.h"

#include <QString>
#include <QDateTime>

#include <expected>
#include <variant>

/// Control messages to gonnectJitsiSession.js
///
/// JS -> GOnnect:
/// { type: "joined", participantId: "<id>", role: "moderator|participant|none" }
/// { type: "roleChanged", participantId: "<id>", role: "moderator|participant|none" }
/// { type: "userJoined", participantId: "<id>", displayName: "<name>", role: "..." }
/// { type: "userLeft", participantId: "<id>" }
/// { type: "displayNameChanged", participantId: "<id>", displayName: "<name>" }
/// { type: "participantMuted", participantId: "<id>", isAudioMuted: bool, isVideoMuted: bool }
/// { type: "handRaised", participantId: "<id>", timestamp: <ms since epoch or 0 if hand lowered> }
/// { type: "dominantSpeaker", participantId: "<id>" }
/// { type: "audioMuted", isMuted: bool }
/// { type: "videoMuted", isMuted: bool }
/// { type: "devices", audioInputs: [{ deviceId, groupId, label }], audioOutputs: [...] }
/// { type: "failed", reason: "<error name>" }
/// { type: "tracks", ...  } - handled by JitsiVideoServer, see JitsiVideoProtocol.h
///
/// GOnnect -> JS:
/// { type: "leave" }
/// { type: "setAudioMuted", isMuted: bool }
/// { type: "setVideoMuted", isMuted: bool }
/// { type: "setAudioInputDevice", deviceId: "<id>" }
/// { type: "setAudioOutputDevice", deviceId: "<id>" }
/// { type: "kickParticipant", participantId: "<id>" }
/// { type: "grantModerator", participantId: "<id>" }
/// { type: "muteAll" }
/// { type: "setHandRaised", isRaised: bool }
/// { type: "receiverConstraints", ...  } - sent by JitsiVideoBridge

namespace JitsiSessionProtocol {

struct Joined
{
    QString participantId;
    ConferenceUser::Role role = ConferenceUser::Role::None;
};
struct PasswordRequired
{
};
struct RoomLockChanged
{
    bool isLocked = false;
};
struct LobbyWaiting
{
};
struct LobbyChanged
{
    bool isEnabled = false;
};
struct LobbyUserJoined
{
    QString participantId;
    QString displayName;
};
struct LobbyUserLeft
{
    QString participantId;
};
struct Failed
{
    QString reason;
};
struct RoleChanged
{
    QString participantId;
    ConferenceUser::Role role = ConferenceUser::Role::None;
};
struct UserJoined
{
    QString participantId;
    QString displayName;
    ConferenceUser::Role role = ConferenceUser::Role::None;
};
struct UserLeft
{
    QString participantId;
};
struct DisplayNameChanged
{
    QString participantId;
    QString displayName;
};
struct ParticipantMuted
{
    QString participantId;
    bool isAudioMuted = false;
    bool isVideoMuted = false;
};
struct HandRaised
{
    QString participantId;
    QDateTime dateTime; // Invalid if hand is lowered
};
struct DominantSpeaker
{
    QString participantId;
};
struct MessageReceived
{
    QString participantId;
    QString displayName;
    QString text;
    QDateTime dateTime;
    bool isFromHistory = false;
};
struct AudioMuted
{
    bool isMuted = false;
};
struct VideoMuted
{
    bool isMuted = false;
};
struct MediaDevice
{
    QString deviceId;
    QString groupId;
    QString label;
};
struct Devices
{
    QList<MediaDevice> audioInputs;
    QList<MediaDevice> audioOutputs;
};

using Event = std::variant<Joined, RoleChanged, UserJoined, UserLeft, DisplayNameChanged,
                           ParticipantMuted, HandRaised, DominantSpeaker, MessageReceived,
                           AudioMuted, VideoMuted, Devices, PasswordRequired, RoomLockChanged,
                           LobbyWaiting, LobbyChanged, LobbyUserJoined, LobbyUserLeft, Failed>;

std::expected<Event, QString> parseEvent(const QString &message);

QString leaveCommand();

QString setAudioMutedCommand(bool isMuted);
QString setVideoMutedCommand(bool isMuted);
QString setAudioInputDeviceCommand(const QString &deviceId);
QString setAudioOutputDeviceCommand(const QString &deviceId);
QString kickParticipantCommand(const QString &participantId);
QString grantModeratorCommand(const QString &participantId);
QString muteAllCommand();
QString setHandRaisedCommand(bool isRaised);
QString sendMessageCommand(const QString &text);
QString joinWithPasswordCommand(const QString &password);
QString setRoomPasswordCommand(const QString &password);
QString setLobbyEnabledCommand(bool isEnabled);
QString answerLobbyUserCommand(const QString &participantId, bool isApproved);

} // namespace JitsiSessionProtocol
