#pragma once

#include "JitsiVideoProtocol.h"
#include "KeyFrameGate.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QThread>
#include <QUrl>

#include <expected>
#include <map>
#include <memory>

class FramePump;
class QWebSocket;
class QWebSocketCorsAuthenticator;
class QWebSocketServer;
class TrackDecoder;

/// WebSocket server for gonnectRemoteVideo.js.
/// See also JitisVideoProtol.h.
/// Lives in its own thread and decodes each track in another thread.
class JitsiVideoServer : public QObject
{
    Q_OBJECT

public:
    JitsiVideoServer(const QUrl &allowedOrigin, const QString &token, QObject *parent = nullptr);
    ~JitsiVideoServer();

    /// Listens on free port of loopback interface
    std::expected<quint16, QString> listen();

    /// Frames of track are decoded into output. W/o output they are dropped.
    void setTrackOutput(const QString &trackId, std::shared_ptr<FramePump> output);

    /// Sent to current and later control connection
    void setReceiverConstraints(const QString &message);

    /// Is dropped when no control connection
    void sendControlMessage(const QString &message);

Q_SIGNALS:
    void tracksReceived(const QList<JitsiVideoProtocol::RemoteTrack> &tracks);
    void controlMessageReceived(const QString &message);

private:
    struct Track
    {
        QString id;
        QWebSocket *socket = nullptr;
        JitsiVideoProtocol::CodecMap codecs;
        KeyFrameGate gate;
        std::unique_ptr<QThread> decoderThread;

        // Lives in decoderThread - deleted when it finishes
        TrackDecoder *decoder = nullptr;
    };

    void authenticateOrigin(QWebSocketCorsAuthenticator *authenticator) const;
    void acceptConnections();
    void acceptControl(QWebSocket *socket);
    void acceptTrack(QWebSocket *socket, const QString &trackId);
    void closeTrack(const QString &trackId);
    Track *findTrack(const QString &trackId, const QWebSocket *socket);

    void receiveCodecs(Track &track, const QString &message);
    void receiveFrame(Track &track, const QByteArray &message);
    void requestKeyFrame(Track &track);

    QUrl m_allowedOrigin;
    QString m_token;
    QWebSocketServer *m_server = nullptr;
    QPointer<QWebSocket> m_controlSocket;
    QString m_receiverConstraints;
    std::map<QString, std::unique_ptr<Track>> m_tracks;
    QHash<QString, std::shared_ptr<FramePump>> m_outputs;
};
