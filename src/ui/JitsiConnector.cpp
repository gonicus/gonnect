#include "JitsiConnector.h"
#include "Notification.h"
#include "ViewHelper.h"
#include "Contact.h"
#include "AuthManager.h"
#include "Theme.h"
#include "HeadsetDeviceProxy.h"
#include "USBDevices.h"
#include "CallHistory.h"
#include "CallHistoryItem.h"
#include "JitsiMediaDevice.h"
#include "AudioManager.h"
#include "VideoManager.h"
#include "SIPAudioDevice.h"
#include "GlobalCallState.h"
#include "GlobalMuteState.h"
#include "FuzzyCompare.h"
#include "NotificationManager.h"
#include "SecretPortal.h"
#include "KeychainSettings.h"
#include "GlobalInfo.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcJitsiConnector, "gonnect.app.JitsiConnector")

QString JitsiConnector::participantRoleToString(const ParticipantRole role)
{
    return QMetaEnum::fromType<ParticipantRole>().valueToKey(static_cast<int>(role));
}

JitsiConnector::JitsiConnector(QObject *parent) : ICallState{ parent }
{
    connect(this, &JitsiConnector::isInRoomChanged, this, [this]() {
        if (isInRoom()) {
            m_establishedDateTime = QDateTime::currentDateTime();
        }
    });

    connect(this, &JitsiConnector::largeVideoParticipantIdChanged, this,
            [this]() { emit executeSetLargeVideoParticipant(m_largeVideoParticipantId); });

    auto &AudioManager = AudioManager::instance();
    connect(&AudioManager, &AudioManager::captureDeviceIdChanged, this,
            &JitsiConnector::transferAudioManagerDevicesToJitsi);
    connect(&AudioManager, &AudioManager::playbackDeviceIdChanged, this,
            &JitsiConnector::transferAudioManagerDevicesToJitsi);

    auto &VideoManager = VideoManager::instance();
    connect(&VideoManager, &VideoManager::selectedDeviceIdChanged, this,
            &JitsiConnector::transferVideoManagerDeviceToJitsi);

    auto headsetProxy = USBDevices::instance().getHeadsetDeviceProxy();
    connect(headsetProxy, &IHeadsetDevice::hookSwitch, this,
            &JitsiConnector::onHeadsetHookSwitchChanged);

    connect(&GlobalMuteState::instance(), &GlobalMuteState::isMutedChangedWithTag, this,
            [this](bool, const QString tag) {
                if (m_isOnHold) {
                    return;
                }
                if (m_muteTag.isEmpty() || m_muteTag != tag) {
                    toggleMute();
                } else if (!m_muteTag.isEmpty() && m_muteTag == tag) {
                    m_muteTag.clear();
                }
            });
}

void JitsiConnector::setJitsiId(QString id)
{
    if (m_jitsiId != id) {
        if (!m_jitsiId.isEmpty()) {
            removeParticipant(m_jitsiId);
        }

        m_jitsiId = id;

        if (!id.isEmpty()) {
            addParticipant(id, jitsiDisplayName());
        }

        emit jitsiIdChanged();
    }
}

void JitsiConnector::setCallHistoryItem(QPointer<CallHistoryItem> callHistoryItem)
{
    m_callHistoryItem = callHistoryItem;
}

void JitsiConnector::apiLoadingFinished()
{
    m_isApiLoadingFinished = true;

    if (m_isToggleScreenSharePending) {
        m_isToggleScreenSharePending = false;
        toggleScreenShare();
    }
}

void JitsiConnector::addError(QString type, QString name, QString message, bool isFatal,
                              QVariantMap details)
{
    qCCritical(lcJitsiConnector) << "Received error from Jitsi Meet" << type << name << message
                                 << "fatal:" << isFatal << "details:" << details;
}

void JitsiConnector::addIncomingMessage(QString fromId, QString nickName, QString message,
                                        QDateTime stamp, bool isPrivateMessage)
{
    qCInfo(lcJitsiConnector) << "Received chat message"
                             << "from" << fromId << nickName << "at" << stamp
                             << "as private message:" << isPrivateMessage << ":" << message;

    m_messages.append({ fromId, nickName, message, stamp, isPrivateMessage });
    emit messageAdded(m_messages.size() - 1);

    // System notification
    AppSettings settings;
    if (settings.value("generic/jitsiChatAsNotifications", true).toBool()) {
        auto notification = new Notification(tr("New chat message"), message,
                                             Notification::Priority::normal, this);
        m_chatNotifications.append(notification);
        NotificationManager::instance().add(notification);

        QObject::connect(notification, &Notification::actionInvoked,
                         []() { emit ViewHelper::instance().showConferenceChat(); });
    }
}

QString JitsiConnector::jitsiHtml()
{
    return QString(R"""(
<!DOCTYPE html>
<html>
<head>
  <script type="text/javascript" src="qrc:///qtwebchannel/qwebchannel.js"></script>
  <script src="%1/external_api.js"></script>
  <style type="text/css">
    html, body {
      height: 100%;
      margin: 0;
    }

    iframe { display: block; }

    #meet {
      height: 100%;
    }

  </style>
</head>
<body>
  <div id="meet"></div>
</body>
</html>
)""")
            .arg(GlobalInfo::instance().jitsiUrl());
}

QString JitsiConnector::jitsiJavascript()
{
    const auto currentUser = ViewHelper::instance().currentUser();
    const auto defaultName = tr("Unnamed participant");

    return QString(R"""(
const options = {
    roomName: '%5',
    jwt: '%2',
    parentNode: document.querySelector('#meet'),
    lang: 'de',
    height: '100%',
    width: '100%',
    userInfo: {
        displayName: "%3"
    },
    configOverwrite: {
        defaultRemoteDisplayName: "%6",
        disableReactions: true,
        disableModeratorIndicator: true,
        disablePolls: true,
        disableProfile: true,
        disableResponsiveTiles: true,
        disableTileView: true,
        disableTileEnlargement: true,
        enableNoisyMicDetection: false,
        hideConferenceSubject: true,
        hideConferenceTimer: true,
        hideParticipantsStats: true,
        mainToolbarButtons: [],
        notifications: [],
        "prejoinConfig.enabled": false,
        readOnlyName: true,
        "recordingService.enabled": false,
        startAudioOnly: false,
        startWithVideoMuted: %7,
        "screenshotCapture.enabled": false,
        "screenShareSettings.desktopSystemAudio": "exclude",
        "tileView.disabled": false,
        toolbarButtons: [],
    },
    interfaceConfigOverwrite: {
        APP_NAME: "GOnnect",
        DEFAULT_BACKGROUND: "%4",
        DISABLE_DOMINANT_SPEAKER_INDICATOR: true,
        RECENT_LIST_ENABLED: false,
        SETTINGS_SECTIONS: [],
        SHOW_JITSI_WATERMARK: false,
        VIDEO_QUALITY_LABEL_DISABLED: true,
    }
}

let jitsiUrl = "%1"

if (jitsiUrl.startsWith("https://")) {
    jitsiUrl = jitsiUrl.slice(8)
}

const api = new JitsiMeetExternalAPI(jitsiUrl, options)

let jitsiConn = null

new QWebChannel(qt.webChannelTransport, function(channel) {
    jitsiConn = channel.objects.jitsiConn


    // Listeners to JitsiConnector

    jitsiConn.executePasswordCommand.connect(pw => {
        api.executeCommand("password", pw)
    })

    jitsiConn.messageSent.connect(msg => {
        api.executeCommand("sendChatMessage", msg)
    })

    jitsiConn.executeLeaveRoomCommand.connect(() => {
        api.executeCommand("hangup")
    })

    jitsiConn.executeEndConferenceCommand.connect(() => {
        api.executeCommand("endConference")
    })

    jitsiConn.executeToggleAudioCommand.connect(() => {
        api.executeCommand("toggleAudio")
    })

    jitsiConn.executeToggleVideoCommand.connect(() => {
        api.executeCommand("toggleVideo")
    })

    jitsiConn.executeToggleShareScreenCommand.connect(() => {
        api.executeCommand("toggleShareScreen")
    })

    jitsiConn.executeToggleTileViewCommand.connect(() => {
        api.executeCommand("toggleTileView")
    })

    jitsiConn.executeToggleRaiseHandCommand.connect(() => {
        api.executeCommand("toggleRaiseHand")
    })

    jitsiConn.executeSetNoiseSupressionCommand.connect((value) => {
        api.executeCommand("setNoiseSuppressionEnabled", { enabled: value })
    })

    jitsiConn.executeSetAudioInputDeviceCommand.connect((deviceId) => {
        api.setAudioInputDevice(undefined, deviceId)
    })

    jitsiConn.executeSetAudioOutputDeviceCommand.connect((deviceId) => {
        api.setAudioOutputDevice(undefined, deviceId)
    })

    jitsiConn.executeSetVideoInputDeviceCommand.connect((deviceId) => {
        api.setVideoInputDevice(undefined, deviceId)
    })

    jitsiConn.executeToggleVirtualBackgroundDialogCommand.connect(() => {
        api.executeCommand("toggleVirtualBackgroundDialog")
    })

    jitsiConn.executeSetLargeVideoParticipant.connect(id => {
        api.executeCommand("setLargeVideoParticipant", id)
    })

    jitsiConn.executeKickParticipantCommand.connect(id => {
        api.executeCommand("kickParticipant", id)
    })

    jitsiConn.executeGrantModeratorCommand.connect(id => {
        api.executeCommand("grantModerator", id)
    })

    jitsiConn.executeMuteAllCommand.connect(id => {
        api.executeCommand("muteEveryone", "audio")
    })

    jitsiConn.executeSetVideoQualityCommand.connect(value => {
        api.executeCommand("setVideoQuality", value)
    })

    jitsiConn.executeSetAudioOnlyCommand.connect(isAudioOnly => {
        api.executeCommand("setAudioOnly", isAudioOnly)
    })

    // Initial function calls on API

    api.isVideoAvailable().then(available => {
        jitsiConn.setIsVideoAvailable(available)
    })

    jitsiConn.apiLoadingFinished()
})

api.addListener("videoConferenceJoined", data => {
    jitsiConn.setJitsiId(api._myUserID)

    api.getRoomsInfo().then(data => {
        for (const room of data.rooms) {
            if (room.isMainRoom) {
                for (const participant of room.participants) {
                    jitsiConn.setParticipantRole(participant.id, participant.role)
                }
            }
        }
    })

    jitsiConn.setVideoQualityInternal(api.getVideoQuality())
})

api.addListener("videoConferenceLeft", () => {
    jitsiConn.leaveRoom()
})

api.addListener("audioMuteStatusChanged", data => {
    jitsiConn.setIsMuted(data.muted)
})

api.addListener("videoAvailabilityChanged", data => {
    jitsiConn.setIsVideoAvailable(data.available)
})

api.addListener("videoMuteStatusChanged", data => {
    jitsiConn.setIsVideoMuted(data.muted)
})

api.addListener("screenSharingStatusChanged", data => {
    jitsiConn.setIsSharingScreen(data.on)
})

api.addListener("tileViewChanged", data => {
    jitsiConn.setIsTileView(data.enabled)
})

api.addListener("deviceListChanged", data => {
    Promise.all([api.getAvailableDevices(), api.getCurrentDevices()]).then(([availableDevices, currentDevices]) => {
        jitsiConn.setJitsiDevices(availableDevices, currentDevices)
    })
})

api.addListener("participantJoined", data => {
    jitsiConn.addParticipant(data.id, data.displayName)
})
api.addListener("participantLeft", data => {
    jitsiConn.removeParticipant(data.id)
})
api.addListener("participantRoleChanged", data => {
    jitsiConn.setParticipantRole(data.id, data.role)
})

api.addListener("errorOccurred", data => {
    jitsiConn.addError(data.type, data.name, data.message, data.isFatal, data.details)
})

api.addListener("incomingMessage", data => {
    const stamp = data.hasOwnProperty("stamp") ? data.stamp : new Date()
    jitsiConn.addIncomingMessage(data.from, data.nick, data.message, stamp, data.privateMessage)
})

api.addListener("passwordRequired", data => {
    jitsiConn.onPasswordRequired()
})

)""")
            .arg(GlobalInfo::instance().jitsiUrl(), // %1
                 AuthManager::instance().jitsiTokenForRoom(m_roomName), // %2
                 currentUser ? currentUser->name() : "", // %3
                 Theme::instance().backgroundColor().name(), // %4
                 m_roomName, // %5
                 defaultName // %6
                 )
            .arg(!m_startWithVideo); // %7
}

void JitsiConnector::enterRoom(const QString &roomName, const QString &displayName,
                               JitsiConnector::MeetingStartFlags startFlags)
{
    qCInfo(lcJitsiConnector).nospace().noquote()
            << "Entering room " << roomName << " (" << displayName << ") with flags:" << startFlags;

    m_startWithVideo = startFlags & JitsiConnector::MeetingStartFlag::VideoActive;

    if (m_callHistoryItem) {
        m_callHistoryItem->addFlags(CallHistoryItem::Type::JitsiMeetCall);
    } else {
        m_callHistoryItem = CallHistory::instance().addHistoryItem(
                CallHistoryItem::Type::JitsiMeetCall, "", roomName);
    }

    setRoomName(roomName);
    setDisplayName(displayName);
    setIsInRoom(true);

    if (m_startWithVideo) {
        toggleVideoMute();
    }

    if (startFlags & JitsiConnector::MeetingStartFlag::ScreenShareActive) {
        if (m_isApiLoadingFinished) {
            toggleScreenShare();
        } else {
            m_isToggleScreenSharePending = true;
        }
    }

    // Maintain fake call info
    ContactInfo contactInfo;
    contactInfo.displayName = displayName.isEmpty() ? roomName : displayName;

    GlobalCallState::instance().setRemoteContactInfo(contactInfo);
}

void JitsiConnector::leaveRoom()
{
    if (m_callHistoryItem) {
        m_callHistoryItem->endCall();
        m_callHistoryItem.clear();
    }

    emit executeLeaveRoomCommand();
    setRoomName("");
    setDisplayName("");
    setIsInRoom(false);
}

void JitsiConnector::terminateRoom()
{
    if (m_callHistoryItem) {
        m_callHistoryItem->endCall();
        m_callHistoryItem.clear();
    }

    emit executeEndConferenceCommand();
    setRoomName("");
    setDisplayName("");
    setIsInRoom(false);
}

QUrl JitsiConnector::roomUrl()
{
    return QUrl(QString("%1/%2").arg(GlobalInfo::instance().jitsiUrl(), roomName()));
}

void JitsiConnector::toggleMute()
{
    emit executeToggleAudioCommand();
}

void JitsiConnector::sendMessage(QString message)
{
    m_messages.append(
            { m_jitsiId, jitsiDisplayName(), message, QDateTime::currentDateTime(), false });
    emit messageAdded(m_messages.size() - 1);
    emit messageSent(message);
}

void JitsiConnector::setIsInRoom(bool value)
{
    if (m_isInRoom != value) {
        m_isInRoom = value;
        emit isInRoomChanged();

        if (value) {
            addCallState(ICallState::State::CallActive | ICallState::State::AudioActive);
            auto &globalMute = GlobalMuteState::instance();
            if (m_isMuted != globalMute.isMuted()) {
                m_muteTag = QUuid::createUuid().toString();
                globalMute.toggleMute(m_muteTag);
            }
        } else {
            setCallState(ICallState::State::Idle);

            m_passwordAlreadyRequested = false;

            qDeleteAll(m_chatNotifications);
            m_chatNotifications.clear();
            m_messages.clear();
            emit messagesReset();

            m_participants.clear();
            emit participantsCleared();
        }
    }
}

void JitsiConnector::setIsOnHold(bool value)
{
    if (m_isOnHold != value) {

        if (value) {
            m_wasVideoMutedBeforeHold = isVideoMuted();

            if (!isMuted()) {
                toggleMute();
            }
            if (!isVideoMuted()) {
                toggleVideoMute();
            }
        } else {
            if (isMuted() != GlobalMuteState::instance().isMuted()) {
                toggleMute();
            }
            if (isVideoMuted() != m_wasVideoMutedBeforeHold) {
                toggleVideoMute();
            }
        }

        m_isOnHold = value;
        emit isOnHoldChanged();
    }
}

void JitsiConnector::setIsPasswordRequired(bool value)
{
    if (m_isPasswordRequired != value) {
        m_isPasswordRequired = value;
        emit isPasswordRequiredChanged();
    }
}

void JitsiConnector::setIsPasswordEntryRequired(bool value)
{
    if (m_isPasswordEntryRequired != value) {
        m_isPasswordEntryRequired = value;
        emit isPasswordEntryRequiredChanged();
    }
}

void JitsiConnector::setRoomPassword(QString value)
{
    if (!isModerator()) {
        qCWarning(lcJitsiConnector)
                << "Cannot set room password because only moderators are allowed to do that";
        return;
    }

    auto &secretPortal = SecretPortal::instance();
    if (secretPortal.isValid()) {
        KeychainSettings settings;
        settings.beginGroup("jitsiRoomPasswords");
        if (value.isEmpty()) {
            settings.remove(m_roomName);
        } else {
            settings.setValue(m_roomName, secretPortal.encrypt(value));
        }
        settings.endGroup();
    }

    emit executePasswordCommand(value);

    if (m_roomPassword != value) {
        m_roomPassword = value;
        emit roomPasswordChanged();
    }

    setIsPasswordRequired(!value.isEmpty());
}

void JitsiConnector::setCurrentAudioInputDevice(JitsiMediaDevice *device)
{
    if (m_currentAudioInputDevice != device) {
        m_currentAudioInputDevice = device;
        emit currentAudioInputDeviceChanged();
    }
}

void JitsiConnector::setCurrentAudioOutputDevice(JitsiMediaDevice *device)
{
    if (m_currentAudioOutputDevice != device) {
        m_currentAudioOutputDevice = device;
        emit currentAudioOutputDeviceChanged();
    }
}

void JitsiConnector::setCurrentVideoInputDevice(JitsiMediaDevice *device)
{
    if (m_currentVideoInputDevice != device) {
        m_currentVideoInputDevice = device;
        emit currentVideoInputDeviceChanged();
    }
}

void JitsiConnector::updateVideoCallState()
{
    if (m_isVideoAvailable && !m_isVideoMuted) {
        addCallState(ICallState::State::VideoActive);
    } else {
        removeCallState(ICallState::State::VideoActive);
    }
}

JitsiMediaDevice *JitsiConnector::createJitsiMediaDevice(const QVariantMap &deviceMap)
{
    if (!deviceMap.contains("deviceId") || !deviceMap.contains("groupId")
        || !deviceMap.contains("kind") || !deviceMap.contains("label")) {
        qCCritical(lcJitsiConnector) << "Keys missing in json object";
        return nullptr;
    }

    JitsiMediaDevice::Type type = JitsiMediaDevice::Type::Unknown;
    const auto typeStr = deviceMap.value("kind").toString();
    if (typeStr == "audioinput") {
        type = JitsiMediaDevice::Type::AudioInput;
    } else if (typeStr == "audiooutput") {
        type = JitsiMediaDevice::Type::AudioOutput;
    } else if (typeStr == "videoinput") {
        type = JitsiMediaDevice::Type::VideoInput;
    } else {
        qCCritical(lcJitsiConnector) << "Unkown device type:" << typeStr;
        return nullptr;
    }

    JitsiMediaDevice *device = new JitsiMediaDevice(deviceMap.value("deviceId").toString(),
                                                    deviceMap.value("groupId").toString(), type,
                                                    deviceMap.value("label").toString(), this);
    return device;
}

JitsiMediaDevice *JitsiConnector::findDevice(const QList<JitsiMediaDevice *> &deviceList,
                                             const QString &deviceId) const
{
    for (const auto device : deviceList) {
        if (device->deviceId() == deviceId) {
            return device;
        }
    }
    return nullptr;
}

void JitsiConnector::setIsMuted(bool value)
{
    if (m_isMuted != value) {
        m_isMuted = value;
        emit isMutedChanged();
    }
}

void JitsiConnector::setIsVideoAvailable(bool value)
{
    if (m_isVideoAvailable != value) {
        m_isVideoAvailable = value;
        emit isVideoAvailableChanged();
        updateVideoCallState();
    }
}

void JitsiConnector::toggleVideoMute()
{
    emit executeToggleVideoCommand();
}

void JitsiConnector::setIsVideoMuted(bool value)
{
    if (m_isVideoMuted != value) {
        m_isVideoMuted = value;
        emit isVideoMutedChanged();
        updateVideoCallState();
    }
}

void JitsiConnector::toggleVirtualBackgroundDialog()
{
    emit executeToggleVirtualBackgroundDialogCommand();
}

void JitsiConnector::toggleScreenShare()
{
    emit executeToggleShareScreenCommand();
}

void JitsiConnector::setIsSharingScreen(bool value)
{
    if (m_isSharingScreen != value) {
        m_isSharingScreen = value;
        emit isSharingScreenChanged();
    }
}

void JitsiConnector::toggleTileView()
{
    setProperty("largeVideoParticipantId", "");
    emit executeToggleTileViewCommand();
}

void JitsiConnector::setIsTileView(bool value)
{
    if (m_isTileView != value) {
        m_isTileView = value;
        emit isTileViewChanged();
    }
}

void JitsiConnector::toggleRaiseHand()
{
    setIsHandRaised(!m_isHandRaised);
    emit executeToggleRaiseHandCommand();
}

void JitsiConnector::toggleNoiseSupression()
{
    m_isNoiseSupression = !m_isNoiseSupression;
    emit executeSetNoiseSupressionCommand(m_isNoiseSupression);
    emit isNoiseSupressionChanged();
}

void JitsiConnector::passwordEntered(const QString &password, bool shouldRemember)
{
    if (shouldRemember) {
        auto &secretPortal = SecretPortal::instance();
        if (secretPortal.isValid()) {
            KeychainSettings settings;
            settings.beginGroup("jitsiRoomPasswords");
            settings.setValue(m_roomName, secretPortal.encrypt(password));
            settings.endGroup();
        }
    }

    setIsPasswordEntryRequired(false);
    emit executePasswordCommand(password);
}

void JitsiConnector::onPasswordRequired()
{
    // First password request - check for persisted password
    if (!m_passwordAlreadyRequested) {
        m_passwordAlreadyRequested = true;

        auto &secretPortal = SecretPortal::instance();
        if (secretPortal.isValid()) {
            KeychainSettings settings;
            settings.beginGroup("jitsiRoomPasswords");
            if (settings.contains(m_roomName)) {
                passwordEntered(secretPortal.decrypt(settings.value(m_roomName).toString()), false);
                return;
            }
            settings.endGroup();
        }
    }

    setIsPasswordRequired(true);
    setIsPasswordEntryRequired(true);
}

void JitsiConnector::setVideoQuality(VideoQuality quality)
{
    if (quality == VideoQuality::AudioOnly) {
        emit executeSetAudioOnlyCommand(true);
    } else {
        if (m_videoQuality == VideoQuality::AudioOnly) {
            emit executeSetAudioOnlyCommand(false);
        }
        emit executeSetVideoQualityCommand(static_cast<uint>(quality));
    }

    if (m_videoQuality != quality) {
        m_videoQuality = quality;
        emit videoQualityChanged();
    }
}

void JitsiConnector::setVideoQualityInternal(uint quality)
{
    const auto conv = static_cast<VideoQuality>(quality);

    if (m_videoQuality != conv) {
        m_videoQuality = conv;
        emit videoQualityChanged();
    }
}

void JitsiConnector::addParticipant(const QString &id, const QString &displayName)
{
    qCDebug(lcJitsiConnector).noquote().nospace()
            << "Adding participant " << displayName << " (" << id << ")";

    // Find insert index
    qsizetype i = 0;

    for (; i < m_participants.size(); ++i) {
        if (displayName.localeAwareCompare(m_participants.at(i).displayName) < 0) {
            break;
        }
    }

    m_participants.insert(i, { id, displayName });
    emit participantAdded(i, id);
    emit numberOfParticipantsChanged();

    addRoomMessage(tr("%1 has joined the conference").arg(displayName));
}

void JitsiConnector::removeParticipant(const QString &id)
{
    for (qsizetype i = 0; i < m_participants.size(); ++i) {
        if (m_participants.at(i).id == id) {
            const QString displayName = m_participants.at(i).displayName;

            qCDebug(lcJitsiConnector).noquote().nospace()
                    << "Removing participant " << displayName << " (" << id << ")";

            m_participants.removeAt(i);
            addRoomMessage(tr("%1 has left the conference").arg(displayName));

            emit participantRemoved(i, id);
            emit numberOfParticipantsChanged();

            return;
        }
    }
}

void JitsiConnector::setParticipantRole(const QString &id, const QString &roleString)
{

    ParticipantRole role = ParticipantRole::None;
    if (roleString == "moderator") {
        role = ParticipantRole::Moderator;
    } else if (roleString == "participant") {
        role = ParticipantRole::Participant;
    }

    if (id == m_jitsiId) {
        if (m_ownRole != role) {
            qCInfo(lcJitsiConnector) << "Own role changed to" << participantRoleToString(role);
            m_ownRole = role;
            emit ownRoleChanged();
        }
    }

    for (qsizetype i = 0; i < m_participants.size(); ++i) {
        auto &participant = m_participants[i];

        if (participant.id == id) {
            participant.role = role;

            qCInfo(lcJitsiConnector).noquote().nospace()
                    << "Jitsi participant " << participant.displayName << " (" << id
                    << ") got new role" << participantRoleToString(role);

            emit participantRoleChanged(i, id, role);
            return;
        }
    }

    qCWarning(lcJitsiConnector).noquote()
            << "Jitsi participant" << id << "got new role" << participantRoleToString(role)
            << "but could not be found in participant list - ignoring";
}

void JitsiConnector::kickParticipant(const QString &id)
{
    emit executeKickParticipantCommand(id);
}

void JitsiConnector::grantParticipantModerator(const QString &id)
{
    emit executeGrantModeratorCommand(id);
}

void JitsiConnector::muteAll()
{
    emit executeMuteAllCommand();
}

void JitsiConnector::setJitsiDevices(const QVariantMap availableDevices,
                                     const QVariantMap currentDevices)
{
    emit beganJitsiDevicesReset();

    m_currentAudioInputDevice = nullptr;
    m_currentAudioOutputDevice = nullptr;
    m_currentVideoInputDevice = nullptr;

    qDeleteAll(m_audioInputDevices);
    m_audioInputDevices.clear();

    qDeleteAll(m_audioOutputDevices);
    m_audioOutputDevices.clear();

    qDeleteAll(m_videoInputDevices);
    m_videoInputDevices.clear();

    const auto audioInputMap = availableDevices.value("audioInput", {}).toList();
    for (const auto &mapEntry : audioInputMap) {
        if (auto deviceObj = createJitsiMediaDevice(mapEntry.toMap())) {
            m_audioInputDevices.append(deviceObj);
        }
    }

    const auto audioOutputMap = availableDevices.value("audioOutput", {}).toList();
    for (const auto &mapEntry : audioOutputMap) {
        if (auto deviceObj = createJitsiMediaDevice(mapEntry.toMap())) {
            m_audioOutputDevices.append(deviceObj);
        }
    }

    const auto videoInputMap = availableDevices.value("videoInput", {}).toList();
    for (const auto &mapEntry : videoInputMap) {
        if (auto deviceObj = createJitsiMediaDevice(mapEntry.toMap())) {
            m_videoInputDevices.append(deviceObj);
        }
    }

    qCInfo(lcJitsiConnector) << "Found"
                             << (m_audioInputDevices.size() + m_audioOutputDevices.size()
                                 + m_videoInputDevices.size())
                             << "Jitsi devices:" << m_audioInputDevices.size()
                             << "audio input devices," << m_audioOutputDevices.size()
                             << "audio output devices and" << m_videoInputDevices.size()
                             << "video input devices";

    if (currentDevices.contains("audioInput")) {
        const auto audioInputMap = currentDevices.value("audioInput").toMap();
        setCurrentAudioInputDevice(
                findDevice(m_audioInputDevices, audioInputMap.value("deviceId").toString()));
    }

    if (currentDevices.contains("audioOutput")) {
        const auto audioOutputMap = currentDevices.value("audioOutput").toMap();
        setCurrentAudioOutputDevice(
                findDevice(m_audioOutputDevices, audioOutputMap.value("deviceId").toString()));
    }

    if (currentDevices.contains("videoInput")) {
        const auto videoInputMap = currentDevices.value("videoInput").toMap();
        setCurrentVideoInputDevice(
                findDevice(m_videoInputDevices, videoInputMap.value("deviceId").toString()));
    }

    emit endedJitsiDevicesReset();

    transferAudioManagerDevicesToJitsi();
    transferVideoManagerDeviceToJitsi();
}

SIPAudioDevice *JitsiConnector::jitsiToSipDevice(const JitsiMediaDevice *jitsiDevice) const
{
    if (!jitsiDevice) {
        return nullptr;
    }

    bool isInput = false;

    switch (jitsiDevice->type()) {
    case JitsiMediaDevice::Type::Unknown:
    case JitsiMediaDevice::Type::VideoInput:
        qCWarning(lcJitsiConnector) << "Can only map audio devices";
        return nullptr;
    case JitsiMediaDevice::Type::AudioInput:
        isInput = true;
        break;
    case JitsiMediaDevice::Type::AudioOutput:
        isInput = false;
        break;
    }

    const auto audioManagerDevices = AudioManager::instance().devices();
    const auto devName = jitsiDevice->label();
    const auto isDefault = jitsiDevice->deviceId() == "default";

    for (auto dev : audioManagerDevices) {
        if (isDefault && dev->isDefault() && dev->isInput() == isInput) {
            return dev;
        }
        if (!isDefault && dev->isInput() == isInput && dev->name() == devName) {
            return dev;
        }
    }
    return nullptr;
}

JitsiMediaDevice *JitsiConnector::sipToJitsiDevice(const SIPAudioDevice *sipDevice) const
{
    if (!sipDevice) {
        return nullptr;
    }

    const auto &devices = sipDevice->isInput() ? m_audioInputDevices : m_audioOutputDevices;
    const auto devName = sipDevice->name();
    const auto isDefault = sipDevice->isDefault();

    for (auto dev : devices) {
        if (isDefault && dev->deviceId() == "default") {
            return dev;
        }
        if (!isDefault && dev->label() == devName) {
            return dev;
        }
    }

    return nullptr;
}

void JitsiConnector::addRoomMessage(QString message, QDateTime stamp)
{
    qCInfo(lcJitsiConnector) << "Adding room 'chat' message at" << stamp << ":" << message;

    m_messages.append({ "room", "", message, stamp, false, true });
    emit messageAdded(m_messages.size() - 1);
}

QString JitsiConnector::jitsiDisplayName() const
{
    const Contact *currUser = ViewHelper::instance().currentUser();
    if (currUser) {
        return currUser->name();
    }

    AppSettings settings;
    return settings.value("jitsi/displayName").toString();
}

void JitsiConnector::selectAudioInputDevice(const QString &deviceId)
{
    auto device = findDevice(m_audioInputDevices, deviceId);
    if (!device) {
        qCCritical(lcJitsiConnector) << "Unkown device for id" << deviceId;
        return;
    }

    setCurrentAudioInputDevice(device);
    emit executeSetAudioInputDeviceCommand(deviceId);
}

void JitsiConnector::selectAudioOutputDevice(const QString &deviceId)
{
    auto device = findDevice(m_audioOutputDevices, deviceId);
    if (!device) {
        qCCritical(lcJitsiConnector) << "Unkown device for id" << deviceId;
        return;
    }

    setCurrentAudioOutputDevice(device);
    emit executeSetAudioOutputDeviceCommand(deviceId);
}

void JitsiConnector::selectVideoInputDevice(const QString &deviceId)
{
    auto device = findDevice(m_videoInputDevices, deviceId);
    if (!device) {
        qCCritical(lcJitsiConnector) << "Unkown device for id" << deviceId;
        return;
    }

    setCurrentVideoInputDevice(device);
    emit executeSetVideoInputDeviceCommand(deviceId);
}

void JitsiConnector::toggleHoldImpl()
{
    setIsOnHold(!m_isOnHold);
}

void JitsiConnector::setIsHandRaised(bool value)
{
    if (m_isHandRaised != value) {
        m_isHandRaised = value;
        emit isHandRaisedChanged();
    }
}

void JitsiConnector::onHeadsetHookSwitchChanged()
{
    const auto headsetProxy = USBDevices::instance().getHeadsetDeviceProxy();
    if (!headsetProxy->getHookSwitch()) {
        leaveRoom();
    }
}

void JitsiConnector::transferAudioManagerDevicesToJitsi()
{
    auto &audioManager = AudioManager::instance();

    const auto inputDeviceId = audioManager.captureDeviceId();
    const auto outputDeviceId = audioManager.playbackDeviceId();
    const auto &devices = audioManager.devices();

    SIPAudioDevice *inputDevice = nullptr;
    SIPAudioDevice *outputDevice = nullptr;

    for (auto dev : devices) {
        if (!inputDevice && dev->isInput() && dev->uniqueId() == inputDeviceId) {
            inputDevice = dev;
        } else if (!outputDevice && dev->isOutput() && dev->uniqueId() == outputDeviceId) {
            outputDevice = dev;
        }

        if (inputDevice && outputDevice) {
            break;
        }
    }

    if (!inputDevice) {
        qCWarning(lcJitsiConnector) << "Could not find input device for id" << inputDeviceId;
    } else if (const auto jitsiInputDevice = sipToJitsiDevice(inputDevice)) {
        selectAudioInputDevice(jitsiInputDevice->deviceId());
    } else {
        qCWarning(lcJitsiConnector)
                << "Could not find jitsi input device equivalent for id" << inputDeviceId;
    }

    if (!outputDevice) {
        qCWarning(lcJitsiConnector) << "Could not find output device for id" << outputDeviceId;
    } else if (const auto jitsiOutputDevice = sipToJitsiDevice(outputDevice)) {
        selectAudioOutputDevice(jitsiOutputDevice->deviceId());
    } else {
        qCWarning(lcJitsiConnector)
                << "Could not find jitsi output device equivalent for id" << outputDeviceId;
    }
}

void JitsiConnector::transferVideoManagerDeviceToJitsi()
{
    const auto videoDevice = VideoManager::instance().selectedDevice();

    if (videoDevice.isNull()) {
        selectVideoInputDevice("");
    }

    // Find correct Jitsi device id for QCameraDevice
    const auto deviceName = videoDevice.description();
    JitsiMediaDevice *bestMatchingDevice = nullptr;
    qreal maxD = 0.0;

    for (auto jitsiDevice : std::as_const(m_videoInputDevices)) {
        const qreal d = FuzzyCompare::jaroWinklerDistance(jitsiDevice->label(), deviceName);
        if (d > maxD) {
            maxD = d;
            bestMatchingDevice = jitsiDevice;
        }
    }

    if (bestMatchingDevice) {
        selectVideoInputDevice(bestMatchingDevice->deviceId());
    }
}

void JitsiConnector::setRoomName(const QString &name)
{
    if (m_roomName != name) {
        m_roomName = name;
        emit roomNameChanged();
    }
}

void JitsiConnector::setDisplayName(const QString &name)
{
    if (m_displayName != name) {
        m_displayName = name;
        emit displayNameChanged();
    }
}
