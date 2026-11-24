#pragma once

#include "ConferenceUser.h"

#include <QString>

#include <expected>
#include <variant>

/// Control messages to gonnectJitsiSession.js
///
/// JS -> GOnnect:
///   {"type":"joined","participantId":"<id>","role":"moderator|participant|none"}
///   {"type":"roleChanged","participantId":"<id>","role":"moderator|participant|none"}
///   {"type":"audioMuted","isMuted":true}
///   {"type":"videoMuted","isMuted":true}
///   {"type":"devices","audioInputs":[{"deviceId","groupId","label"}],"audioOutputs":[...]}
///   {"type":"failed","reason":"<error name>"}
///   {"type":"tracks", ... } - handled by JitsiVideoServer, see JitsiVideoProtocol.h
///
/// GOnnect -> JS:
///   {"type":"leave"}
///   {"type":"setAudioMuted","isMuted":true}
///   {"type":"setVideoMuted","isMuted":true}
///   {"type":"setAudioInputDevice","deviceId":"<id>"}
///   {"type":"setAudioOutputDevice","deviceId":"<id>"}
///   {"type":"receiverConstraints", ... } - sent by JitsiVideoBridge

namespace JitsiSessionProtocol {

struct Joined
{
    QString participantId;
    ConferenceUser::Role role = ConferenceUser::Role::None;
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

using Event = std::variant<Joined, RoleChanged, AudioMuted, VideoMuted, Devices, Failed>;

std::expected<Event, QString> parseEvent(const QString &message);

QString leaveCommand();

QString setAudioMutedCommand(bool isMuted);
QString setVideoMutedCommand(bool isMuted);
QString setAudioInputDeviceCommand(const QString &deviceId);
QString setAudioOutputDeviceCommand(const QString &deviceId);

} // namespace JitsiSessionProtocol
