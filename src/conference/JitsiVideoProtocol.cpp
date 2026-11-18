#include "JitsiVideoProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

#include <optional>

using namespace Qt::Literals::StringLiterals;

namespace {
constexpr uint MaxPayloadType = 127;

std::expected<QJsonObject, QString> parseMessage(const QString &message,
                                                 QLatin1StringView expectedType)
{
    QJsonParseError error;

    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (!document.isObject()) {
        return std::unexpected(QStringLiteral("Invalid JSON: %1").arg(error.errorString()));
    }

    QJsonObject object = document.object();
    const QString type = object.value("type"_L1).toString();
    if (type != expectedType) {
        return std::unexpected(QStringLiteral("Unexpected message type \"%1\"").arg(type));
    }
    return object;
}

std::optional<VideoCodec> codecFromMimeType(const QString &mimeType)
{
    const QString name = mimeType.section(u'/', 1).toUpper();
    if (name == "VP8"_L1) {
        return VideoCodec::VP8;
    }
    if (name == "VP9"_L1) {
        return VideoCodec::VP9;
    }
    if (name == "AV1"_L1) {
        return VideoCodec::AV1;
    }
    return std::nullopt;
}
} // namespace

std::expected<EncodedFrame, QString> JitsiVideoProtocol::parseFrame(QByteArrayView message,
                                                                    const CodecMap &codecs)
{
    if (message.size() < FrameHeaderSize) {
        return std::unexpected(QStringLiteral("Truncated frame of %1 bytes").arg(message.size()));
    }

    const auto *header = reinterpret_cast<const uchar *>(message.data());
    if (header[0] != Version) {
        return std::unexpected(QStringLiteral("Unsupported protocol version %1").arg(header[0]));
    }

    const quint8 payloadType = header[2];
    const auto codec = codecs.constFind(payloadType);
    if (codec == codecs.cend()) {
        return std::unexpected(QStringLiteral("Unknown payload type %1").arg(payloadType));
    }

    EncodedFrame frame;
    frame.codec = *codec;
    frame.isKeyFrame = (header[1] & KeyFrameFlag) != 0;
    frame.rtpTimestamp = qFromLittleEndian<quint32>(header + 4);
    frame.size =
            QSize(qFromLittleEndian<quint16>(header + 8), qFromLittleEndian<quint16>(header + 10));
    frame.payload = message.sliced(FrameHeaderSize).toByteArray();
    return frame;
}

std::expected<JitsiVideoProtocol::CodecMap, QString>
JitsiVideoProtocol::parseCodecs(const QString &message)
{
    const auto object = parseMessage(message, "codecs"_L1);
    if (!object) {
        return std::unexpected(object.error());
    }

    CodecMap result;
    const auto codecs = object->value("codecs"_L1).toObject();

    for (auto it = codecs.constBegin(); it != codecs.constEnd(); ++it) {
        bool isNumber = false;
        const uint payloadType = it.key().toUInt(&isNumber);
        const auto codec = codecFromMimeType(it.value().toString());

        if (isNumber && payloadType <= MaxPayloadType && codec) {
            result.insert(static_cast<quint8>(payloadType), *codec);
        }
    }
    return result;
}

std::expected<QList<JitsiVideoProtocol::RemoteTrack>, QString>
JitsiVideoProtocol::parseTracks(const QString &message)
{
    const auto object = parseMessage(message, "tracks"_L1);
    if (!object) {
        return std::unexpected(object.error());
    }

    QList<RemoteTrack> tracks;
    const auto entries = object->value("tracks"_L1).toArray();
    for (const auto &entry : entries) {
        const auto track = entry.toObject();
        const bool isScreenShare = track.value("videoType"_L1).toString() == "desktop"_L1;

        const RemoteTrack remoteTrack{ .trackId = track.value("trackId"_L1).toString(),
                                       .sourceName = track.value("sourceName"_L1).toString(),
                                       .participantId = track.value("participantId"_L1).toString(),
                                       .type = isScreenShare
                                               ? ConferenceVideoSource::Type::ScreenShare
                                               : ConferenceVideoSource::Type::Camera,
                                       .isMuted = track.value("isMuted"_L1).toBool() };

        if (remoteTrack.trackId.isEmpty() || remoteTrack.sourceName.isEmpty()
            || remoteTrack.participantId.isEmpty()) {
            continue;
        }
        tracks.append(remoteTrack);
    }
    return tracks;
}

QString JitsiVideoProtocol::keyFrameRequestMessage()
{
    return QStringLiteral(R"({"type":"keyFrameRequest"})");
}
