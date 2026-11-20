#include "JitsiVideoServer.h"
#include "FramePump.h"
#include "OriginHelper.h"
#include "TrackDecoder.h"

#include <QLoggingCategory>
#include <QUrlQuery>
#include <QWebSocket>
#include <QWebSocketCorsAuthenticator>
#include <QWebSocketServer>

#include <utility>

Q_LOGGING_CATEGORY(lcJitsiVideoServer, "gonnect.conference.video.server")

using namespace Qt::Literals::StringLiterals;

namespace {

// Max frames waiting for decoder
constexpr int MaxQueuedFrames = 4;

constexpr quint64 MaxMessageSize = 4 * 1024 * 1024;
} // namespace

JitsiVideoServer::JitsiVideoServer(const QUrl &allowedOrigin, const QString &token, QObject *parent)
    : QObject{ parent }, m_allowedOrigin{ allowedOrigin }, m_token{ token }
{
}

JitsiVideoServer::~JitsiVideoServer()
{
    while (!m_tracks.empty()) {
        closeTrack(m_tracks.begin()->first);
    }
}

std::expected<quint16, QString> JitsiVideoServer::listen()
{
    m_server =
            new QWebSocketServer(u"GOnnect remot video"_s, QWebSocketServer::NonSecureMode, this);
    connect(m_server, &QWebSocketServer::originAuthenticationRequired, this,
            &JitsiVideoServer::authenticateOrigin);
    connect(m_server, &QWebSocketServer::newConnection, this, &JitsiVideoServer::acceptConnections);

    if (!m_server->listen(QHostAddress::LocalHost, 0)) {
        return std::unexpected(m_server->errorString());
    }

    qCInfo(lcJitsiVideoServer) << "Listening on port" << m_server->serverPort();
    return m_server->serverPort();
}

void JitsiVideoServer::setTrackOutput(const QString &trackId, std::shared_ptr<FramePump> output)
{
    const bool hasOutput = output != nullptr;
    if (hasOutput) {
        m_outputs.insert(trackId, std::move(output));
    } else {
        m_outputs.remove(trackId);
    }

    const auto it = m_tracks.find(trackId);
    if (it == m_tracks.end()) {
        return;
    }

    // Decoder references belong to previous track owner
    Track &track = *it->second;
    track.gate.requireKeyFrame();
    if (hasOutput) {
        requestKeyFrame(track);
    }
}

void JitsiVideoServer::setReceiverConstraints(const QString &message)
{
    m_receiverConstraints = message;
    if (m_controlSocket) {
        m_controlSocket->sendTextMessage(message);
    }
}

void JitsiVideoServer::authenticateOrigin(QWebSocketCorsAuthenticator *authenticator) const
{
    const bool isAllowed =
            OriginHelper::isSameOrigin(QUrl(authenticator->origin()), m_allowedOrigin);
    if (!isAllowed) {
        qCWarning(lcJitsiVideoServer)
                << "Rejected connection from origin" << authenticator->origin();
    }
    authenticator->setAllowed(isAllowed);
}

void JitsiVideoServer::acceptConnections()
{
    while (m_server->hasPendingConnections()) {
        QWebSocket *socket = m_server->nextPendingConnection();

        // Never log url as it contains the token
        const QUrl url = socket->requestUrl();
        const QUrlQuery query(url);

        if (query.queryItemValue(u"token"_s) != m_token) {
            qCWarning(lcJitsiVideoServer) << "Rejected connection with invalid token";
            socket->close(QWebSocketProtocol::CloseCodePolicyViolated);
            socket->deleteLater();
            continue;
        }

        socket->setMaxAllowedIncomingFrameSize(MaxMessageSize);

        const QString trackId = query.queryItemValue(u"id"_s, QUrl::FullyDecoded);
        if (url.path() == "/control"_L1) {
            acceptControl(socket);
        } else if (url.path() == "/track"_L1 && !trackId.isEmpty()) {
            acceptTrack(socket, trackId);
        } else {
            qCWarning(lcJitsiVideoServer) << "Rejected connection to" << url.path();
            socket->close(QWebSocketProtocol::CloseCodePolicyViolated);
            socket->deleteLater();
        }
    }
}

void JitsiVideoServer::acceptControl(QWebSocket *socket)
{
    // Reloaded frame replaces its predecessor
    if (m_controlSocket) {
        m_controlSocket->disconnect(this);
        m_controlSocket->close();
        m_controlSocket->deleteLater();
    }
    m_controlSocket = socket;
    qCInfo(lcJitsiVideoServer) << "Control connection established";

    connect(socket, &QWebSocket::textMessageReceived, this, [this](const QString &message) {
        const auto tracks = JitsiVideoProtocol::parseTracks(message);
        if (!tracks) {
            qCWarning(lcJitsiVideoServer) << "Invalid control message:" << tracks.error();
            return;
        }
        Q_EMIT tracksReceived(*tracks);
    });

    connect(socket, &QWebSocket::disconnected, this, [this, socket]() {
        qCInfo(lcJitsiVideoServer) << "Control connection closed";
        socket->deleteLater();

        // Cannot assign track w/o control connection
        Q_EMIT tracksReceived({});
    });

    // Reloaded frame must not fall back to Jitsi default constraints
    if (!m_receiverConstraints.isEmpty()) {
        socket->sendTextMessage(m_receiverConstraints);
    }
}

void JitsiVideoServer::acceptTrack(QWebSocket *socket, const QString &trackId)
{
    // Restarted transform replaces its predecessor
    closeTrack(trackId);

    auto track = std::make_unique<Track>();
    track->id = trackId;
    track->socket = socket;
    track->decoderThread = std::make_unique<QThread>();
    track->decoderThread->setObjectName(u"decoder %1"_s.arg(trackId));

    track->decoder = new TrackDecoder(trackId);
    track->decoder->moveToThread(track->decoderThread.get());

    connect(track->decoderThread.get(), &QThread::finished, track->decoder, &QObject::deleteLater);

    connect(track->decoder, &TrackDecoder::decodeFailed, this, [this, trackId, socket]() {
        if (Track *failed = findTrack(trackId, socket)) {
            requestKeyFrame(*failed);
        }
    });

    connect(socket, &QWebSocket::textMessageReceived, this,
            [this, trackId, socket](const QString &message) {
                if (Track *receiving = findTrack(trackId, socket)) {
                    receiveCodecs(*receiving, message);
                }
            });

    connect(socket, &QWebSocket::binaryMessageReceived, this,
            [this, trackId, socket](const QByteArray &message) {
                if (Track *receiving = findTrack(trackId, socket)) {
                    receiveFrame(*receiving, message);
                }
            });

    connect(socket, &QWebSocket::disconnected, this, [this, trackId, socket]() {
        if (findTrack(trackId, socket)) {
            closeTrack(trackId);
        }
    });

    track->decoderThread->start();
    m_tracks.emplace(trackId, std::move(track));
    qCInfo(lcJitsiVideoServer) << "Track connected" << trackId;
}

void JitsiVideoServer::closeTrack(const QString &trackId)
{
    const auto it = m_tracks.find(trackId);
    if (it == m_tracks.end()) {
        return;
    }

    const std::unique_ptr<Track> track = std::move(it->second);
    m_tracks.erase(it);

    track->socket->disconnect(this);
    track->socket->close();
    track->socket->deleteLater();

    track->decoderThread->quit();
    track->decoderThread->wait();
    qCInfo(lcJitsiVideoServer) << "Track closed" << trackId;
}

JitsiVideoServer::Track *JitsiVideoServer::findTrack(const QString &trackId,
                                                     const QWebSocket *socket)
{
    const auto it = m_tracks.find(trackId);
    if (it != m_tracks.end() && it->second->socket == socket) {
        return it->second.get();
    }
    return nullptr;
}

void JitsiVideoServer::receiveCodecs(Track &track, const QString &message)
{
    const auto codecs = JitsiVideoProtocol::parseCodecs(message);
    if (!codecs) {
        qCWarning(lcJitsiVideoServer) << "Invalid message for" << track.id << codecs.error();
        return;
    }

    track.codecs = *codecs;
    qCInfo(lcJitsiVideoServer) << "Received" << track.codecs.size() << "decodable codecs for"
                               << track.id;

    // Frames before were dropped
    if (track.gate.isWaitingForKeyFrame() && m_outputs.contains(track.id)) {
        requestKeyFrame(track);
    }
}

void JitsiVideoServer::receiveFrame(Track &track, const QByteArray &message)
{
    if (track.codecs.isEmpty()) {
        // Key frame requested once codecs received
        return;
    }

    auto frame = JitsiVideoProtocol::parseFrame(message, track.codecs);
    if (!frame) {
        qCDebug(lcJitsiVideoServer) << "Dropping frame of" << track.id << frame.error();
        return;
    }

    const std::shared_ptr<FramePump> output = m_outputs.value(track.id);
    if (!output) {
        // Unassigned tracks are not shown -> key frame requested on assignment
        track.gate.requireKeyFrame();
        return;
    }

    if (track.decoder->queuedFrames() > MaxQueuedFrames) {
        if (!track.gate.isWaitingForKeyFrame()) {
            qCDebug(lcJitsiVideoServer) << "Decoder of" << track.id << "is overloaded";
        }
        track.gate.requireKeyFrame();
    }

    if (!track.gate.accept(*frame)) {
        requestKeyFrame(track);
        return;
    }

    track.decoder->enqueue(std::move(*frame), output);
}

void JitsiVideoServer::requestKeyFrame(Track &track)
{
    track.gate.requireKeyFrame();
    if (track.gate.shouldRequestKeyFrame(KeyFrameGate::Clock::now())) {
        track.socket->sendTextMessage(JitsiVideoProtocol::keyFrameRequestMessage());
    }
}
