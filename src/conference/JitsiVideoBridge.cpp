#include "JitsiVideoBridge.h"
#include "JitsiVideoServer.h"
#include "SecretGenerator.h"

#include <QLoggingCategory>
#include <QSet>

Q_LOGGING_CATEGORY(lcJitsiVideoBridge, "gonnect.conference.video.bridge")

using namespace Qt::Literals::StringLiterals;

namespace {
constexpr quint8 TokenLength = 32;
constexpr std::chrono::milliseconds ConstraintsDelay{ 100 };
} // namespace

JitsiVideoBridge::JitsiVideoBridge(ConferenceVideoModel *model, const QUrl &allowedOrigin,
                                   QObject *parent)
    : QObject{ parent }, m_model{ model }, m_allowedOrigin{ allowedOrigin }
{
    m_networkThread.setObjectName(u"JitsiVideoBrdige"_s);

    m_constraintsTimer.setSingleShot(true);
    m_constraintsTimer.setInterval(ConstraintsDelay);
    connect(&m_constraintsTimer, &QTimer::timeout, this,
            &JitsiVideoBridge::sendReceiverConstraints);

    if (m_model) {
        connect(m_model, &ConferenceVideoModel::requestedHeightsChanged, this,
                &JitsiVideoBridge::scheduleReceiverConstraints);
        connect(m_model, &ConferenceVideoModel::rowsRemoved, this,
                &JitsiVideoBridge::scheduleReceiverConstraints);
        connect(m_model, &ConferenceVideoModel::modelReset, this,
                &JitsiVideoBridge::scheduleReceiverConstraints);
    }
}

JitsiVideoBridge::~JitsiVideoBridge()
{
    stop();
}

std::expected<void, QString> JitsiVideoBridge::start()
{
    if (m_server) {
        return {};
    }

    m_token = SecretGenerator::generateSecret(TokenLength);
    m_server = new JitsiVideoServer(m_allowedOrigin, m_token);
    m_server->moveToThread(&m_networkThread);

    connect(&m_networkThread, &QThread::finished, m_server, &QObject::deleteLater);
    connect(m_server, &JitsiVideoServer::tracksReceived, this, &JitsiVideoBridge::applyTracks);
    m_networkThread.start();

    std::expected<quint16, QString> port;
    QMetaObject::invokeMethod(
            m_server, [server = m_server, &port]() { port = server->listen(); },
            Qt::BlockingQueuedConnection);

    if (!port) {
        stop();
        return std::unexpected(QStringLiteral("Cannot listen: %1").arg(port.error()));
    }

    m_endpoint = QUrl(u"ws://127.0.0.1:%1"_s.arg(*port));
    qCInfo(lcJitsiVideoBridge) << "Started";
    return {};
}

void JitsiVideoBridge::stop()
{
    if (!m_server) {
        return;
    }

    m_networkThread.quit();
    m_networkThread.wait();
    m_server = nullptr;

    m_constraintsTimer.stop();
    m_sentConstraints.clear();
    m_endpoint.clear();
    m_token.clear();
    m_sourceNameOfTrack.clear();

    if (m_model) {
        m_model->clear();
    }

    qCInfo(lcJitsiVideoBridge) << "Stopped";
}

void JitsiVideoBridge::applyTracks(const QList<JitsiVideoProtocol::RemoteTrack> &tracks)
{
    // Snapshots may still be queued after stop()
    if (!m_server || !m_model) {
        return;
    }

    QSet<QString> sourceNames;
    QHash<QString, QString> sourceNameOfTrack;

    for (const auto &track : tracks) {
        auto *source = m_model->addSource(track.sourceName, track.participantId, track.type);
        source->setIsMuted(track.isMuted);
        sourceNames.insert(track.sourceName);
        sourceNameOfTrack.insert(track.trackId, track.sourceName);
    }

    const QList<ConferenceVideoSource *> sources = m_model->sources();
    for (const auto *source : sources) {
        if (!sourceNames.contains(source->id())) {
            m_model->removeSource(source->id());
        }
    }

    for (auto it = sourceNameOfTrack.cbegin(); it != sourceNameOfTrack.cend(); ++it) {
        if (m_sourceNameOfTrack.value(it.key()) != it.value()) {
            setTrackOutput(it.key(), m_model->source(it.value())->framePump());
        }
    }
    for (auto it = m_sourceNameOfTrack.cbegin(); it != m_sourceNameOfTrack.cend(); ++it) {
        if (!sourceNameOfTrack.contains(it.key())) {
            setTrackOutput(it.key(), nullptr);
        }
    }
    m_sourceNameOfTrack = sourceNameOfTrack;
}

void JitsiVideoBridge::setTrackOutput(const QString &trackId, std::shared_ptr<FramePump> output)
{
    QMetaObject::invokeMethod(
            m_server,
            [server = m_server, trackId, output = std::move(output)]() mutable {
                server->setTrackOutput(trackId, std::move(output));
            },
            Qt::QueuedConnection);
}

void JitsiVideoBridge::scheduleReceiverConstraints()
{
    if (m_server && !m_constraintsTimer.isActive()) {
        m_constraintsTimer.start();
    }
}

void JitsiVideoBridge::sendReceiverConstraints()
{
    if (!m_server || !m_model) {
        return;
    }

    QList<JitsiVideoProtocol::ReceiverConstraint> constraints;
    const QList<ConferenceVideoSource *> sources = m_model->sources();

    for (const auto *source : sources) {
        if (source->requestedHeight() <= 0) {
            continue;
        }

        const bool isScreenShare = source->type() == ConferenceVideoSource::Type::ScreenShare;
        constraints.append({ .sourceName = source->id(),
                             .maxHeight = isScreenShare
                                     ? JitsiVideoProtocol::MaxReceiverHeight
                                     : JitsiVideoProtocol::qualityHeight(source->requestedHeight()),
                             .isOnStage = isScreenShare });
    }

    const QString message = JitsiVideoProtocol::receiverConstraintsMessage(constraints);
    if (message == m_sentConstraints) {
        return;
    }
    m_sentConstraints = message;

    qCDebug(lcJitsiVideoBridge) << "Receiver constraints" << message;

    QMetaObject::invokeMethod(
            m_server, [server = m_server, message]() { server->setReceiverConstraints(message); },
            Qt::QueuedConnection);
}
