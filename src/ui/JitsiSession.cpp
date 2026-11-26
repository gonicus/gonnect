#include "JitsiSession.h"
#include "ConferenceChatRoom.h"
#include "ErrorBus.h"
#include "GlobalMuteState.h"
#include "JitsiConnector.h"
#include "JitsiVideoBridge.h"

#include <QLoggingCategory>
#include <QVariantMap>

#include <variant>

Q_LOGGING_CATEGORY(lcJitsiSession, "gonnect.conference.session")

using namespace Qt::Literals::StringLiterals;

namespace {

QVariantList toVariantList(const QList<JitsiSessionProtocol::MediaDevice> &devices,
                           const QString &type)
{
    QVariantList list;
    for (const auto &device : devices) {
        list.append(QVariantMap{
                { u"deviceId"_s, device.deviceId },
                { u"groupId"_s, device.groupId },
                { u"kind"_s, type },
                { u"label"_s, device.label },
        });
    }
    return list;
}
} // namespace

JitsiSession::JitsiSession(JitsiConnector *connector, JitsiVideoBridge *bridge, QObject *parent)
    : QObject{ parent }, m_connector{ connector }, m_bridge{ bridge }
{
    connect(m_bridge, &JitsiVideoBridge::controlMessageReceived, this, &JitsiSession::receiveEvent);

    connect(m_connector, &JitsiConnector::executeLeaveRoomCommand, this, [this]() {
        m_isJoined = false;
        send(JitsiSessionProtocol::leaveCommand());
    });
    connect(m_connector, &JitsiConnector::executeToggleAudioCommand, this, [this]() {
        sendIfJoined(JitsiSessionProtocol::setAudioMutedCommand(!m_connector->isAudioMuted()));
    });
    connect(m_connector, &JitsiConnector::executeToggleVideoCommand, this, [this]() {
        sendIfJoined(JitsiSessionProtocol::setVideoMutedCommand(m_connector->isVideoMuted()));
    });
    connect(m_connector, &JitsiConnector::executeSetAudioInputDeviceCommand, this,
            [this](const QString &deviceId) {
                send(JitsiSessionProtocol::setAudioInputDeviceCommand(deviceId));
            });
    connect(m_connector, &JitsiConnector::executeSetAudioOutputDeviceCommand, this,
            [this](const QString &deviceId) {
                send(JitsiSessionProtocol::setAudioOutputDeviceCommand(deviceId));
            });
    connect(m_connector, &JitsiConnector::executeKickUserCommand, this,
            [this](const QString &participantId) {
                sendIfJoined(JitsiSessionProtocol::kickParticipantCommand(participantId));
            });
    connect(m_connector, &JitsiConnector::executeGrantModeratorCommand, this,
            [this](const QString &participantId) {
                sendIfJoined(JitsiSessionProtocol::grantModeratorCommand(participantId));
            });
    connect(m_connector, &JitsiConnector::executeMuteAllCommand, this,
            [this]() { sendIfJoined(JitsiSessionProtocol::muteAllCommand()); });
    connect(m_connector, &JitsiConnector::executeToggleRaiseHandCommand, this, [this]() {
        sendIfJoined(JitsiSessionProtocol::setHandRaisedCommand(!m_connector->isHandRaised()));
    });
    connect(m_connector, &JitsiConnector::executePasswordCommand, this,
            [this](const QString &password) {
                if (m_isJoined) {
                    send(JitsiSessionProtocol::setRoomPasswordCommand(password));
                } else {
                    send(JitsiSessionProtocol::joinWithPasswordCommand(password));
                }
            });

    if (const auto *chatRoom = qobject_cast<const ConferenceChatRoom *>(m_connector->chatRoom())) {
        connect(chatRoom, &ConferenceChatRoom::sendMessageRequested, this,
                [this](const QString &text) {
                    sendIfJoined(JitsiSessionProtocol::sendMessageCommand(text));
                });
    }
}

void JitsiSession::receiveEvent(const QString &message)
{
    const auto event = JitsiSessionProtocol::parseEvent(message);
    if (!event) {
        qCWarning(lcJitsiSession) << "Invalid event:" << event.error();
        return;
    }

    std::visit([this](const auto &value) { handle(value); }, *event);
}

void JitsiSession::handle(const JitsiSessionProtocol::Joined &joined)
{
    qCInfo(lcJitsiSession) << "Joined conference as" << joined.participantId;

    m_connector->apiLoadingFinishedInternal();
    m_connector->setJitsiId(joined.participantId);
    m_connector->setUserRole(joined.participantId, joined.role);
    m_connector->setVideoAvailableInternal(true);
    m_isJoined = true;

    const bool isAudioMuted = m_connector->isOnHold() || GlobalMuteState::instance().isMuted();
    send(JitsiSessionProtocol::setAudioMutedCommand(isAudioMuted));
    send(JitsiSessionProtocol::setVideoMutedCommand(m_connector->isVideoMuted()));
}

void JitsiSession::handle(const JitsiSessionProtocol::RoleChanged &roleChanged)
{
    m_connector->setUserRole(roleChanged.participantId, roleChanged.role);
}

void JitsiSession::handle(const JitsiSessionProtocol::UserJoined &userJoined)
{
    m_connector->addUser(userJoined.participantId, userJoined.displayName);
    m_connector->setUserRole(userJoined.participantId, userJoined.role);
}

void JitsiSession::handle(const JitsiSessionProtocol::UserLeft &userLeft)
{
    m_connector->removeUser(userLeft.participantId);
}

void JitsiSession::handle(const JitsiSessionProtocol::DisplayNameChanged &displayNameChanged)
{
    m_connector->setUserDisplayName(displayNameChanged.participantId,
                                    displayNameChanged.displayName);
}

void JitsiSession::handle(const JitsiSessionProtocol::ParticipantMuted &participantMuted)
{
    qCDebug(lcJitsiSession) << "Participant" << participantMuted.participantId
                            << "audio muted:" << participantMuted.isAudioMuted
                            << "video muted:" << participantMuted.isVideoMuted;

    m_connector->setUserAudioMuted(participantMuted.participantId, participantMuted.isAudioMuted);
    m_connector->setUserVideoMuted(participantMuted.participantId, participantMuted.isVideoMuted);
}

void JitsiSession::handle(const JitsiSessionProtocol::HandRaised &handRaised)
{
    qCDebug(lcJitsiSession) << "Participant" << handRaised.participantId << "hand raised at"
                            << handRaised.dateTime;

    m_connector->setUserHandRaisedDateTime(handRaised.participantId, handRaised.dateTime);
    if (handRaised.participantId == m_connector->ownId()) {
        m_connector->setIsHandRaisedInternal(handRaised.dateTime.isValid());
    }
}

void JitsiSession::handle(const JitsiSessionProtocol::DominantSpeaker &dominantSpeaker)
{
    qCDebug(lcJitsiSession) << "Dominant speaker:" << dominantSpeaker.participantId;
    m_connector->setDominantSpeakerById(dominantSpeaker.participantId);
}

void JitsiSession::handle(const JitsiSessionProtocol::MessageReceived &message)
{
    m_connector->addIncomingMessage(message.participantId, message.displayName, message.text,
                                    message.dateTime, false, message.isFromHistory);
}

void JitsiSession::handle(const JitsiSessionProtocol::AudioMuted &audioMuted)
{
    m_connector->setAudioMuted(audioMuted.isMuted);
    m_connector->setUserAudioMuted(m_connector->ownId(), audioMuted.isMuted);
}

void JitsiSession::handle(const JitsiSessionProtocol::VideoMuted &videoMuted)
{
    m_connector->setVideoMutedInternal(videoMuted.isMuted);
    m_connector->setUserVideoMuted(m_connector->ownId(), videoMuted.isMuted);
}

void JitsiSession::handle(const JitsiSessionProtocol::Devices &devices)
{
    m_connector->setJitsiDevices({
            { u"audioInput"_s, toVariantList(devices.audioInputs, u"audioinput"_s) },
            { u"audioOutput"_s, toVariantList(devices.audioOutputs, u"audiooutput"_s) },
    });
}

void JitsiSession::handle(const JitsiSessionProtocol::PasswordRequired &)
{
    qCInfo(lcJitsiSession) << "Conference requires a password";
    m_connector->onPasswordRequired();
}

void JitsiSession::handle(const JitsiSessionProtocol::RoomLockChanged &roomLockChanged)
{
    qCInfo(lcJitsiSession) << "Room locked:" << roomLockChanged.isLocked;
    m_connector->setIsRoomLockedInternal(roomLockChanged.isLocked);
}

void JitsiSession::handle(const JitsiSessionProtocol::Failed &failed)
{
    qCWarning(lcJitsiSession) << "Conference failed:" << failed.reason;
    m_isJoined = false;

    if (!m_connector->isInConference()) {
        return;
    }

    ErrorBus::instance().error(tr("Conference failed: %1").arg(failed.reason));
    m_connector->leaveConference();
}

void JitsiSession::send(const QString &command)
{
    m_bridge->sendControlMessage(command);
}

void JitsiSession::sendIfJoined(const QString &command)
{
    if (m_isJoined) {
        send(command);
    }
}
