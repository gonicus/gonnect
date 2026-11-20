#pragma once

#include "ConferenceVideoModel.h"
#include "JitsiVideoProtocol.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <expected>
#include <memory>

class FramePump;
class JitsiVideoServer;

/// Receives remote video of Jitsi iframe and feeds into conference video model
class JitsiVideoBridge : public QObject
{
    Q_OBJECT

public:
    JitsiVideoBridge(ConferenceVideoModel *model, const QUrl &allowedOrigin,
                     QObject *parent = nullptr);
    ~JitsiVideoBridge() override;

    std::expected<void, QString> start();

    /// Also removes all sources from model
    void stop();

    bool isRunning() const { return m_server != nullptr; }

    /// Base URL of WebSocket server, e.g. "ws://127.0.0.1:12345
    QUrl endpoint() const { return m_endpoint; }

    /// Required as "token" query item of each connection
    QString token() const { return m_token; }

private Q_SLOTS:
    void scheduleReceiverConstraints();
    void sendReceiverConstraints();

private:
    void applyTracks(const QList<JitsiVideoProtocol::RemoteTrack> &tracks);
    void setTrackOutput(const QString &trackId, std::shared_ptr<FramePump> output);

    QPointer<ConferenceVideoModel> m_model;
    QUrl m_allowedOrigin;
    QThread m_networkThread;

    // Lives in m_networkThread, is deleted when that finishes
    JitsiVideoServer *m_server = nullptr;

    QUrl m_endpoint;
    QString m_token;
    QHash<QString, QString> m_sourceNameOfTrack;

    QTimer m_constraintsTimer;
    QString m_sentConstraints;
};
