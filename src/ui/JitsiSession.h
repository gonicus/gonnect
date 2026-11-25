#pragma once

#include "JitsiSessionProtocol.h"

#include <QObject>

class JitsiConnector;
class JitsiVideoBridge;

/// Drives JitsiConnector by a custom lib-jitsi-meet page
class JitsiSession : public QObject
{
    Q_OBJECT

public:
    JitsiSession(JitsiConnector *connector, JitsiVideoBridge *bridge, QObject *parent = nullptr);

private:
    void receiveEvent(const QString &message);
    void handle(const JitsiSessionProtocol::Joined &joined);
    void handle(const JitsiSessionProtocol::RoleChanged &roleChanged);
    void handle(const JitsiSessionProtocol::UserJoined &userJoined);
    void handle(const JitsiSessionProtocol::UserLeft &userLeft);
    void handle(const JitsiSessionProtocol::DisplayNameChanged &displayNameChanged);
    void handle(const JitsiSessionProtocol::ParticipantMuted &participantMuted);
    void handle(const JitsiSessionProtocol::HandRaised &handRaised);
    void handle(const JitsiSessionProtocol::DominantSpeaker &dominantSpeaker);
    void handle(const JitsiSessionProtocol::MessageReceived &message);
    void handle(const JitsiSessionProtocol::AudioMuted &audioMuted);
    void handle(const JitsiSessionProtocol::VideoMuted &videoMuted);
    void handle(const JitsiSessionProtocol::Devices &devices);
    void handle(const JitsiSessionProtocol::Failed &failed);

    void send(const QString &command);
    void sendIfJoined(const QString &command);

    JitsiConnector *m_connector = nullptr;
    JitsiVideoBridge *m_bridge = nullptr;
    bool m_isJoined = false;
};
