#pragma once

#include "ConferenceVideoSource.h"
#include "EncodedFrame.h"

#include <QByteArrayView>
#include <QHash>
#include <QList>
#include <QString>

#include <expected>

/// Messages between GOnnect and gonnectRemoteVideo.js in the Jitsi iframe.
///
/// The iframe opens a control connection ("/control") and sends a snapshot of all remote video
/// tracks whenever they change:
///   {"type":"tracks","tracks":[{"trackId","sourceName","participantId","videoType","isMuted"}]}
///
/// GOnnect answers on the same connection with the video it wants to receive. Sources not listed
/// are not received at all:
///   {"type":"receiverConstraints","constraints":{"<sourceName>":{"maxHeight":360}},
///    "onStageSources":["<sourceName>"]}
///
/// Each remote video track is tapped by a worker with its own connection ("/track?id=<trackId>").
/// It first sends the negotiated codecs as {"type":"codecs","codecs":{"<payloadType>":"<mime>"}},
/// then each encoded frame as binary message with a little endian header:
///   0  u8   protocol version
///   1  u8   flags, bit 0 = key frame
///   2  u8   RTP payload type
///   3  u8   reserved
///   4  u32  RTP timestamp
///   8  u16  width (key frames only)
///   10 u16  height (key frames only)
namespace JitsiVideoProtocol {
constexpr quint8 Version = 2;
constexpr qsizetype FrameHeaderSize = 12;
constexpr quint8 KeyFrameFlag = 0x01;

/// Requested height of screen shares; must be readable in any tile size
constexpr int MaxReceiverHeight = 2160;

/// RTP payload type -> codec
using CodecMap = QHash<quint8, VideoCodec>;

struct RemoteTrack
{
    QString trackId;
    QString sourceName;
    QString participantId;
    ConferenceVideoSource::Type type = ConferenceVideoSource::Type::Camera;
    bool isMuted = false;
};

struct ReceiverConstraint
{
    QString sourceName;
    int maxHeight = 0;

    // Received with priority; may exceed maxHeight if no smaller layer exists
    bool isOnStage = false;
};

std::expected<EncodedFrame, QString> parseFrame(QByteArrayView message, const CodecMap &codecs);

std::expected<CodecMap, QString> parseCodecs(const QString &message);

std::expected<QList<RemoteTrack>, QString> parseTracks(const QString &message);

QString keyFrameRequestMessage();

/// Largest standard layer height not above display height
/// minimum 180
/// 0 if not displayed
int qualityHeight(int displayedHeight);

QString receiverConstraintsMessage(const QList<ReceiverConstraint> &constraints);

} // namespace JitsiVideoProtocol
