#include "JitsiSession.h"
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

void JitsiSession::handle(const JitsiSessionProtocol::AudioMuted &audioMuted)
{
    m_connector->setAudioMuted(audioMuted.isMuted);
}

void JitsiSession::handle(const JitsiSessionProtocol::VideoMuted &videoMuted)
{
    m_connector->setVideoMutedInternal(videoMuted.isMuted);
}

void JitsiSession::handle(const JitsiSessionProtocol::Devices &devices)
{
    m_connector->setJitsiDevices({
            { u"audioInput"_s, toVariantList(devices.audioInputs, u"audioinput"_s) },
            { u"audioOutput"_s, toVariantList(devices.audioOutputs, u"audiooutput"_s) },
    });
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
