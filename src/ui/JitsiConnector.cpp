#include "JitsiConnector.h"
#include "NetworkHelper.h"
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
#include "GlobalMuteState.h"
#include "FuzzyCompare.h"
#include "NotificationManager.h"
#include "GlobalInfo.h"
#include "ConferenceParticipant.h"
#include "DateEventManager.h"
#include "Credentials.h"
#include "GlobalCallState.h"
#include "ChatMessage.h"

#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QtConcurrent>

Q_LOGGING_CATEGORY(lcJitsiConnector, "gonnect.app.JitsiConnector")

#define GONNECT_ASSERT(condition, failMessage)                              \
    if (!(condition)) {                                                     \
        qCCritical(lcJitsiConnector) << "Assertion failed:" << failMessage; \
        return;                                                             \
    }

JitsiConnector::JitsiConnector(QObject *parent) : IConferenceConnector{ parent }
{
    m_chatRoom = new ConferenceChatRoom(this);

    connect(this, &JitsiConnector::isInConferenceChanged, this, [this]() {
        if (isInConference()) {
            m_establishedDateTime = QDateTime::currentDateTime();
        }
    });

    connect(this, &IConferenceConnector::largeVideoParticipantChanged, this, [this]() {
        Q_EMIT executeSetLargeVideoParticipant(
                m_largeVideoParticipant ? m_largeVideoParticipant->id() : "");
    });

    auto &audioManager = AudioManager::instance();
    connect(&audioManager, &AudioManager::captureDeviceIdChanged, this,
            &JitsiConnector::transferAudioManagerDevicesToJitsi);
    connect(&audioManager, &AudioManager::playbackDeviceIdChanged, this,
            &JitsiConnector::transferAudioManagerDevicesToJitsi);

    auto &videoManager = VideoManager::instance();
    connect(&videoManager, &VideoManager::selectedDeviceIdChanged, this,
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

    checkJitsiBackendFeatures();
}

JitsiConnector::~JitsiConnector()
{
    qDeleteAll(m_chatNotifications);
}

QString JitsiConnector::ownDisplayName()
{
    return ViewHelper::instance().currentUserName();
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

        Q_EMIT ownIdChanged();
    }
}

void JitsiConnector::setCallHistoryItem(QPointer<CallHistoryItem> callHistoryItem)
{
    m_callHistoryItem = callHistoryItem;
}

void JitsiConnector::apiLoadingFinishedInternal()
{
    m_isApiLoadingFinished = true;

    if (m_isToggleScreenSharePending) {
        m_isToggleScreenSharePending = false;

        // It can sometimes crash the JS API, when the toggleScreenShare command is executed too
        // early. Since there is no reliable event to listen to, a random timer must be used here to
        // defer it such that Jitsi Meet is hopefully ready...
        QTimer::singleShot(2000, this, [this]() { setSharingScreen(true); });
    }

    Q_EMIT isInitializedChanged();
}

void JitsiConnector::setVideoMutedInternal(bool value)
{
    if (m_isVideoMuted != value) {
        m_isVideoMuted = value;
        Q_EMIT isVideoMutedChanged();
        updateVideoCallState();
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

    ChatMessage::Flags flags = ChatMessage::Flag::Unknown;
    if (isPrivateMessage) {
        flags |= ChatMessage::Flag::PrivateMessage;
    }
    if (fromId == ownId()) {
        flags |= ChatMessage::Flag::OwnMessage;
    }

    auto msgObj = new ChatMessage("", fromId, nickName, message, stamp, flags);
    m_chatRoom->addMessage(msgObj);

    // System notification
    AppSettings settings;
    if (settings.value("generic/jitsiChatAsNotifications", true).toBool()) {
        auto notification = new Notification(tr("New chat message"), message,
                                             Notification::Priority::normal, this);
        notification->setIcon(":/icons/gonnect.svg");

        m_chatNotifications.append(notification);
        NotificationManager::instance().add(notification);

        QObject::connect(notification, &Notification::actionInvoked,
                         []() { Q_EMIT ViewHelper::instance().showConferenceChat(); });
    }
}

QString JitsiConnector::jitsiHtmlInternal()
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

QString JitsiConnector::jitsiJavascriptInternal()
{
    const auto defaultName = tr("Unnamed participant");
    auto &authManager = AuthManager::instance();

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

    jitsiConn.chatRoom().then(
        chatRoom => {
            chatRoom.sendMessageRequested.connect(msg => {
                api.executeCommand("sendChatMessage", msg)
            })
        }
    ).catch(err => {
        throw new Error("Unable to receive chat room object:", err)
    })

    jitsiConn.executePasswordCommand.connect(pw => {
        api.executeCommand("password", pw)
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

    jitsiConn.executeToggleSubtitlesCommand.connect(() => {
        api.executeCommand("toggleSubtitles")
    })

    jitsiConn.executeToggleWhiteboardCommand.connect(() => {
        api.executeCommand("toggleWhiteboard")
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
        jitsiConn.setVideoAvailableInternal(available)
    })

    jitsiConn.apiLoadingFinishedInternal()
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
    jitsiConn.leaveConference()
})

api.addListener("audioMuteStatusChanged", data => {
    jitsiConn.setAudioMuted(data.muted)
})

api.addListener("videoAvailabilityChanged", data => {
    jitsiConn.setVideoAvailableInternal(data.available)
})

api.addListener("videoMuteStatusChanged", data => {
    jitsiConn.setVideoMutedInternal(data.muted)
})

api.addListener("screenSharingStatusChanged", data => {
    jitsiConn.setIsSharingScreenInternal(data.on)
})

api.addListener("tileViewChanged", data => {
    jitsiConn.setIsTileViewInternal(data.enabled)
})

api.addListener("deviceListChanged", data => {
    api.getAvailableDevices().then(availableDevices => {
        jitsiConn.setJitsiDevices(availableDevices)
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
    jitsiConn.addError(data.type, data.name, data.message, !!data.isFatal, data?.details ?? {})
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
                 authManager.isJitsiAuthRequired() ? authManager.jitsiTokenForRoom(m_roomName)
                                                   : QByteArray(), // %2
                 ownDisplayName(), // %3
                 Theme::instance().backgroundColor().name(), // %4
                 m_roomName, // %5
                 defaultName // %6
                 )
            .arg(!m_startWithVideo); // %7
}

void JitsiConnector::toggleMute()
{
    m_didExecuteAudioMuteToggle = true;
    Q_EMIT executeToggleAudioCommand();
}

void JitsiConnector::checkJitsiBackendFeatures()
{
    auto manager = new QNetworkAccessManager(this);

    QNetworkRequest request;
    request.setUrl(QUrl(QString("%1/config.js").arg(GlobalInfo::instance().jitsiUrl())));

    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::errorOccurred, this, [](QNetworkReply::NetworkError err) {
        qCCritical(lcJitsiConnector) << "Error on fetching config js:" << err;
    });
    connect(reply, &QNetworkReply::sslErrors, this, [](const QList<QSslError> &errors) {
        qCCritical(lcJitsiConnector) << "SSL errors on fetching config js:";
        for (const auto &err : errors) {
            qCCritical(lcJitsiConnector) << "  " << err.errorString();
        }
    });
    connect(reply, &QIODevice::readyRead, this, [reply, this]() {
        static const QRegularExpression whiteboardRegex(
                R"(whiteboard\s*(?:=|:)\s*{\s*enabled\s*:\s*true)");
        static const QRegularExpression etherpadRegex(R"(etherpad_base\s*(?:=|:)\s*['"])");
        static const QRegularExpression dialInConfCodeUrl(
                R"(dialInConfCodeUrl\s*(?:=|:)\s*['"](?<url>.*)['"])");
        static const QRegularExpression dialInNumbersUrl(
                R"(dialInNumbersUrl\s*(?:=|:)\s*['"](?<url>.*)['"])");

        const auto txt = reply->readAll();

        setHasWhiteboard(whiteboardRegex.match(txt).hasMatch());
        setHasTextpad(etherpadRegex.match(txt).hasMatch());

        const auto dialInConfCodeUrlMatch = dialInConfCodeUrl.match(txt);
        if (dialInConfCodeUrlMatch.hasMatch()) {
            m_dialInConfCodeUrl = dialInConfCodeUrlMatch.captured("url");
        }

        const auto dialInNumbersUrlMatch = dialInNumbersUrl.match(txt);
        if (dialInNumbersUrlMatch.hasMatch()) {
            m_dialInNumbersUrl = dialInNumbersUrlMatch.captured("url");
        }

        if (hasDialIn()) {
            Q_EMIT hasDialInChanged();
        }
    });
}

void JitsiConnector::setHasWhiteboard(bool value)
{
    if (hasCapability(Capability::Whiteboard) && m_hasWhiteboard != value) {
        m_hasWhiteboard = value;
        Q_EMIT hasWhiteboardChanged();
    }
}

void JitsiConnector::setHasTextpad(bool value)
{
    if (hasCapability(Capability::Textpad) && m_hasTextpad != value) {
        m_hasTextpad = value;
        Q_EMIT hasTextpadChanged();
    }
}

void JitsiConnector::setLargeVideoParticipantById(const QString &id)
{
    for (const auto participant : std::as_const(m_participants)) {
        if (participant->id() == id) {
            setLargeVideoParticipant(participant);
            return;
        }
    }
    setLargeVideoParticipant(nullptr);
}

void JitsiConnector::setIsInConference(bool value)
{
    if (m_isInConference != value) {
        m_isInConference = value;
        Q_EMIT isInConferenceChanged();

        if (value) {
            addCallState(ICallState::State::CallActive | ICallState::State::AudioActive);
            auto &globalMute = GlobalMuteState::instance();

            if (m_isAudioMuted != globalMute.isMuted()) {
                toggleMute();
            }
        } else {
            setCallState(ICallState::State::Idle);

            m_passwordAlreadyRequested = false;

            qDeleteAll(m_chatNotifications);
            m_chatNotifications.clear();

            m_participants.clear();
            Q_EMIT participantsCleared();
        }
    }
}

void JitsiConnector::setIsPasswordRequired(bool value)
{
    if (m_isPasswordRequired != value) {
        m_isPasswordRequired = value;
        Q_EMIT isPasswordRequiredChanged();
    }
}

void JitsiConnector::setRoomPassword(QString value)
{
    if (ownRole() != ConferenceParticipant::Role::Moderator) {
        qCWarning(lcJitsiConnector)
                << "Cannot set room password because only moderators are allowed to do that";
        return;
    }

    QString authGroup = "jitsiRoomPasswords/" + m_roomName;

    Credentials::instance().set(
            authGroup, value, [this, value, authGroup](bool error, const QString &data) {
                if (error) {
                    qCCritical(lcJitsiConnector) << "failed to set credentials:" << data;
                } else {
                    Q_EMIT executePasswordCommand(value);

                    if (m_roomPassword != value) {
                        m_roomPassword = value;
                        Q_EMIT roomPasswordChanged();
                    }

                    setIsPasswordRequired(!value.isEmpty());
                }
            });
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

void JitsiConnector::setVideoAvailableInternal(bool value)
{
    if (m_isVideoAvailable != value) {
        m_isVideoAvailable = value;
        Q_EMIT isVideoAvailableChanged();
        updateVideoCallState();
    }
}

void JitsiConnector::setIsSharingScreenInternal(bool value)
{
    if (m_isSharingScreen != value) {
        m_isSharingScreen = value;
        Q_EMIT isSharingScreenChanged();
    }
}

void JitsiConnector::setIsTileViewInternal(bool value)
{
    if (m_isTileView != value) {
        m_isTileView = value;
        Q_EMIT isTileViewChanged();
    }
}

void JitsiConnector::onPasswordRequired()
{
    // First password request - check for persisted password
    if (!m_passwordAlreadyRequested) {
        m_passwordAlreadyRequested = true;

        QString authGroup = "jitsiRoomPasswords/" + m_roomName;

        Credentials::instance().get(authGroup, [this, authGroup](bool error, const QString &data) {
            if (error) {
                qCCritical(lcJitsiConnector) << "failed to get credentials:" << data;
            } else {
                if (!data.isEmpty()) {
                    enterPassword(data, false);
                    return;
                }
            }

            setIsPasswordRequired(true);
        });
    } else {
        setIsPasswordRequired(true);
    }
}

void JitsiConnector::setVideoQuality(VideoQuality quality)
{
    if (quality == VideoQuality::AudioOnly) {
        Q_EMIT executeSetAudioOnlyCommand(true);
    } else {
        if (m_videoQuality == VideoQuality::AudioOnly) {
            Q_EMIT executeSetAudioOnlyCommand(false);
        }

        uint numVal = 0;

        switch (quality) {

        case IConferenceConnector::VideoQuality::AudioOnly:
            numVal = 0;
            break;
        case IConferenceConnector::VideoQuality::Minimum:
        case IConferenceConnector::VideoQuality::Low:
            numVal = 180;
            break;
        case IConferenceConnector::VideoQuality::Average:
            numVal = 360;
            break;
        case IConferenceConnector::VideoQuality::High:
            numVal = 720;
            break;
        case IConferenceConnector::VideoQuality::Maximum:
            numVal = 2160;
            break;
        }

        Q_EMIT executeSetVideoQualityCommand(numVal);
    }

    if (m_videoQuality != quality) {
        m_videoQuality = quality;
        Q_EMIT videoQualityChanged();
    }
}

void JitsiConnector::setVideoQualityInternal(uint quality)
{
    VideoQuality conv = VideoQuality::AudioOnly;

    if (quality >= 2160) {
        conv = VideoQuality::Maximum;
    } else if (quality >= 720) {
        conv = VideoQuality::High;
    } else if (quality >= 360) {
        conv = VideoQuality::Average;
    } else if (quality >= 180) {
        conv = VideoQuality::Low;
    }

    if (m_videoQuality != conv) {
        m_videoQuality = conv;
        Q_EMIT videoQualityChanged();
    }
}

void JitsiConnector::showVirtualBackgroundDialog()
{
    Q_EMIT executeToggleVirtualBackgroundDialogCommand();
}

void JitsiConnector::addParticipant(const QString &id, const QString &displayName)
{
    qCDebug(lcJitsiConnector).noquote().nospace()
            << "Adding participant " << displayName << " (" << id << ")";

    // Find insert index
    qsizetype i = 0;

    for (; i < m_participants.size(); ++i) {
        if (displayName.localeAwareCompare(m_participants.at(i)->displayName()) < 0) {
            break;
        }
    }

    auto participant = new ConferenceParticipant(id, displayName,
                                                 ConferenceParticipant::Role::Participant, this);
    m_participants.insert(i, participant);
    Q_EMIT participantAdded(i, participant);
    Q_EMIT numberOfParticipantsChanged();

    addRoomMessage(tr("%1 has joined the conference").arg(displayName));
}

void JitsiConnector::removeParticipant(const QString &id)
{
    for (qsizetype i = 0; i < m_participants.size(); ++i) {
        if (m_participants.at(i)->id() == id) {
            auto participant = m_participants.at(i);
            const QString displayName = participant->displayName();

            qCDebug(lcJitsiConnector).noquote().nospace()
                    << "Removing participant " << displayName << " (" << id << ")";

            m_participants.removeAt(i);
            addRoomMessage(tr("%1 has left the conference").arg(displayName));

            Q_EMIT participantRemoved(i, participant);
            Q_EMIT numberOfParticipantsChanged();

            return;
        }
    }
}

void JitsiConnector::setParticipantRole(const QString &id, const QString &roleString)
{
    using Role = ConferenceParticipant::Role;

    Role role = Role::None;
    if (roleString == "moderator") {
        role = Role::Moderator;
    } else if (roleString == "participant") {
        role = Role::Participant;
    }

    if (id == m_jitsiId) {
        if (m_ownRole != role) {
            qCInfo(lcJitsiConnector) << "Own role changed to"
                                     << ConferenceParticipant::participantRoleToString(role);
            m_ownRole = role;
            Q_EMIT ownRoleChanged();
        }
    }

    for (qsizetype i = 0; i < m_participants.size(); ++i) {
        auto &participant = m_participants[i];

        if (participant->id() == id) {
            participant->setRole(role);

            qCInfo(lcJitsiConnector).noquote().nospace()
                    << "Jitsi participant " << participant->displayName() << " (" << id
                    << ") got new role" << ConferenceParticipant::participantRoleToString(role);

            Q_EMIT participantRoleChanged(i, participant, role);
            return;
        }
    }

    qCWarning(lcJitsiConnector).noquote()
            << "Jitsi participant" << id << "got new role"
            << ConferenceParticipant::participantRoleToString(role)
            << "but could not be found in participant list - ignoring";
}

void JitsiConnector::muteAll()
{
    Q_EMIT executeMuteAllCommand();
}

void JitsiConnector::setJitsiDevices(const QVariantMap availableDevices)
{
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
    m_chatRoom->addMessage(
            new ChatMessage("", "room", "", message, stamp, ChatMessage::Flag::SystemMessage));
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

    Q_EMIT executeSetAudioInputDeviceCommand(deviceId);
}

void JitsiConnector::selectAudioOutputDevice(const QString &deviceId)
{
    auto device = findDevice(m_audioOutputDevices, deviceId);
    if (!device) {
        qCCritical(lcJitsiConnector) << "Unkown device for id" << deviceId;
        return;
    }

    Q_EMIT executeSetAudioOutputDeviceCommand(deviceId);
}

void JitsiConnector::selectVideoInputDevice(const QString &deviceId)
{
    auto device = findDevice(m_videoInputDevices, deviceId);
    if (!device) {
        qCCritical(lcJitsiConnector) << "Unkown device for id" << deviceId;
        return;
    }

    Q_EMIT executeSetVideoInputDeviceCommand(deviceId);
}

void JitsiConnector::toggleHoldImpl()
{
    setOnHold(!m_isOnHold);
}

void JitsiConnector::setHandRaised(bool value)
{
    if (m_isHandRaised != value) {
        m_isHandRaised = value;
        Q_EMIT isHandRaisedChanged();
        Q_EMIT executeToggleRaiseHandCommand();
    }
}

void JitsiConnector::onHeadsetHookSwitchChanged()
{
    const auto headsetProxy = USBDevices::instance().getHeadsetDeviceProxy();
    if (!headsetProxy->getHookSwitch()) {
        leaveConference();
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

void JitsiConnector::setConferenceName(const QString &name)
{
    if (m_roomName != name) {
        m_roomName = name;
        Q_EMIT conferenceNameChanged();
    }
}

void JitsiConnector::setDisplayName(const QString &name)
{
    if (m_displayName != name) {
        m_displayName = name;
        Q_EMIT displayNameChanged();
    }
}

ContactInfo JitsiConnector::remoteContactInfo() const
{
    ContactInfo contactInfo;
    contactInfo.displayName = m_displayName.isEmpty() ? m_roomName : m_displayName;
    return contactInfo;
}

bool JitsiConnector::hasCapability(const Capability capabilityToCheck) const
{
    const static QSet<Capability> m_capabilites = {
        Capability::AudioMute,
        Capability::ChatInCall,
        Capability::Holdable,
        Capability::RaiseHand,
        Capability::ScreenShare,
        Capability::Sharable,
        Capability::Subtitles,
        Capability::MuteAll,
        Capability::NoiseSuppression,
        Capability::ParticipantRoles,
        Capability::ParticipantKickable,
        Capability::RoomPassword,
        Capability::ShareUrl,
        Capability::Textpad,
        Capability::TileView,
        Capability::Whiteboard,
        Capability::VideoMute,
        Capability::VideoQualityAdjustable,
    };
    return m_capabilites.contains(capabilityToCheck);
}

void JitsiConnector::toggleWhiteboard()
{
    Q_EMIT executeToggleWhiteboardCommand();
}

bool JitsiConnector::hasDialIn() const
{
    return m_dialInConfCodeUrl.isValid() && m_dialInNumbersUrl.isValid();
}

void JitsiConnector::requestDialInInfo()
{
    if (!hasDialIn()) {
        qCWarning(lcJitsiConnector) << "Dial in feature is not available";
        return;
    }
    if (!isInConference()) {
        qCWarning(lcJitsiConnector) << "Currently no active conference";
        return;
    }

    QUrlQuery numbersQuery(m_dialInNumbersUrl);
    numbersQuery.addQueryItem("conference", m_roomName);
    QUrl numbersUrl = m_dialInNumbersUrl;
    numbersUrl.setQuery(numbersQuery);
    auto numberFuture = NetworkHelper::fetchUrlAsJson(numbersUrl);

    QUrlQuery codeQuery(m_dialInConfCodeUrl);
    codeQuery.addQueryItem("conference", m_roomName);
    QUrl codeUrl = m_dialInConfCodeUrl;
    codeUrl.setQuery(codeQuery);
    auto pinFuture = NetworkHelper::fetchUrlAsJson(codeUrl);

    QFuture<void> combinedFuture = QtConcurrent::run([this, numberFuture, pinFuture]() {
        const QJsonDocument numberDoc = numberFuture.result();
        const QJsonDocument pinDoc = pinFuture.result();

        // Phone numbers
        GONNECT_ASSERT(!numberDoc.isNull(), "JSON result for phone number is invalid")
        GONNECT_ASSERT(numberDoc.isObject(), "Received json must be an object")
        const auto numberObj = numberDoc.object();
        GONNECT_ASSERT(numberObj.contains("numbers"), "Number JSON must have field 'numbers'")
        const auto numbersVal = numberObj.value("numbers");
        GONNECT_ASSERT(numbersVal.isObject(), "'numbers' field must be an object")
        const auto numbersObj = numbersVal.toObject();
        GONNECT_ASSERT(!numbersObj.isEmpty(), "'numbers' field must be empty")

        QVariantMap numbersMap;
        for (auto it = numbersObj.constBegin(); it != numbersObj.constEnd(); ++it) {
            GONNECT_ASSERT(it.value().isArray(), "Numbers entry must be an array")
            numbersMap.insert(it.key(), it.value().toArray().toVariantList());
        }

        // Conference code
        GONNECT_ASSERT(!pinDoc.isNull(), "JSON result for conference code is invalid")
        GONNECT_ASSERT(pinDoc.isObject(), "Received json must be an object")
        const auto pinObj = pinDoc.object();
        GONNECT_ASSERT(pinObj.contains("id"), "JSON must contain field 'id'")
        GONNECT_ASSERT(pinObj.contains("conference"), "JSON must contain field 'conference'")

        Q_EMIT dialInfoReceived(numbersMap, QString::number(pinObj.value("id").toInteger()));
    });
}

void JitsiConnector::joinConference(const QString &conferenceId, const QString &displayName,
                                    IConferenceConnector::StartFlags startFlags)
{
    qCInfo(lcJitsiConnector).nospace().noquote() << "Entering conference " << conferenceId << " ("
                                                 << displayName << ") with flags:" << startFlags;

    auto &globalCallState = GlobalCallState::instance();
    globalCallState.holdAllCalls(this);

    if (isOnHold()) {
        toggleHold();
    }

    DateEventManager::instance().removeNotificationByRoomName(conferenceId);

    m_startWithVideo = startFlags & IConferenceConnector::StartFlag::VideoActive;

    if (m_callHistoryItem) {
        m_callHistoryItem->addFlags(CallHistoryItem::Type::JitsiMeetCall);
    } else {
        m_callHistoryItem = CallHistory::instance().addHistoryItem(
                CallHistoryItem::Type::JitsiMeetCall, "", conferenceId);
    }

    setConferenceName(conferenceId);
    setDisplayName(displayName);
    setIsInConference(true);
    setVideoMuted(!m_startWithVideo);

    if (startFlags & IConferenceConnector::StartFlag::ScreenShareActive) {
        if (isInitialized()) {
            setSharingScreen(true);
        } else {
            m_isToggleScreenSharePending = true;
        }
    }

    // Create notification about ongoing conference
    m_inConferenceNotification = new Notification(tr("Active conference"), this->displayName(),
                                                  Notification::Priority::normal, this);
    m_inConferenceNotification->setDisplayHint(Notification::tray
                                               | Notification::hideContentOnLockScreen);
    m_inConferenceNotification->setCategory("call.ongoing");
    m_inConferenceNotification->addButton(tr("Hang up"), "hangup", "call.hang-up", {});
    m_inConferenceNotification->setIcon(":/icons/gonnect.svg");

    QString ref = NotificationManager::instance().add(m_inConferenceNotification);
    connect(m_inConferenceNotification, &Notification::actionInvoked, this,
            [this, ref](QString action, QVariantList) {
                if (action == "hangup") {
                    NotificationManager::instance().remove(ref);
                    leaveConference();
                }
            });

    connect(m_inConferenceNotification, &QObject::destroyed, this, [this](QObject *obj) {
        if (m_inConferenceNotification == obj) {
            m_inConferenceNotification = nullptr;
        }
    });

    Q_EMIT GlobalCallState::instance().callStarted(true);
}

void JitsiConnector::enterPassword(const QString &password, bool rememberPassword)
{
    if (rememberPassword) {
        QString authGroup = "jitsiRoomPasswords/" + m_roomName;

        Credentials::instance().set(
                authGroup, password, [this, password, authGroup](bool error, const QString &data) {
                    if (error) {
                        qCCritical(lcJitsiConnector) << "failed to set credentials:" << data;
                    } else {
                        Q_EMIT executePasswordCommand(password);

                        if (m_roomPassword != password) {
                            m_roomPassword = password;
                            Q_EMIT roomPasswordChanged();
                        }

                        setIsPasswordRequired(false);

                        Q_EMIT executePasswordCommand(password);
                    }
                });
    }
}

void JitsiConnector::leaveConference()
{
    if (m_callHistoryItem) {
        m_callHistoryItem->endCall();
        m_callHistoryItem.clear();
    }

    if (m_inConferenceNotification) {
        NotificationManager::instance().remove(m_inConferenceNotification->id());
        m_inConferenceNotification->deleteLater();
        m_inConferenceNotification = nullptr;
    }

    m_chatRoom->clear();

    Q_EMIT executeLeaveRoomCommand();
    setConferenceName("");
    setDisplayName("");
    setIsInConference(false);
    Q_EMIT GlobalCallState::instance().callEnded(true);
    GlobalCallState::instance().unholdOtherCall();
}

void JitsiConnector::terminateConference()
{
    if (m_callHistoryItem) {
        m_callHistoryItem->endCall();
        m_callHistoryItem.clear();
    }

    if (m_inConferenceNotification) {
        NotificationManager::instance().remove(m_inConferenceNotification->id());
        m_inConferenceNotification->deleteLater();
        m_inConferenceNotification = nullptr;
    }

    m_chatRoom->clear();

    Q_EMIT executeEndConferenceCommand();
    setConferenceName("");
    setDisplayName("");
    setIsInConference(false);
    Q_EMIT GlobalCallState::instance().callEnded(true);
    GlobalCallState::instance().unholdOtherCall();
}

void JitsiConnector::setOnHold(bool shallHold)
{
    if (m_isOnHold != shallHold) {

        if (shallHold) {
            m_wasVideoMutedBeforeHold = isVideoMuted();

            if (!isAudioMuted()) {
                toggleMute();
            }
            if (!isVideoMuted()) {
                setVideoMuted(!isVideoMuted());
            }
        } else {
            GlobalCallState::instance().holdAllCalls(this);

            if (isAudioMuted() != GlobalMuteState::instance().isMuted()) {
                toggleMute();
            }
            if (isVideoMuted() != m_wasVideoMutedBeforeHold) {
                setVideoMuted(!isVideoMuted());
            }
        }

        m_isOnHold = shallHold;
        Q_EMIT isOnHoldChanged();
    }
}

void JitsiConnector::setAudioMuted(bool value)
{
    if (m_isAudioMuted != value) {
        m_isAudioMuted = value;
        Q_EMIT isAudioMutedChanged();
    }

    if (!m_didExecuteAudioMuteToggle) {
        m_muteTag = QUuid::createUuid().toString();
        GlobalMuteState::instance().toggleMute(m_muteTag);
    }

    m_didExecuteAudioMuteToggle = false;
}

void JitsiConnector::setVideoMuted(bool shallMute)
{
    if (m_isVideoMuted != shallMute) {
        m_isVideoMuted = shallMute;
        Q_EMIT executeToggleVideoCommand();
        Q_EMIT isVideoMutedChanged();
        updateVideoCallState();
    }
}

void JitsiConnector::setTileView(bool showTileView)
{
    if (m_isTileView != showTileView) {
        setLargeVideoParticipant(nullptr);
        Q_EMIT executeToggleTileViewCommand();
    }
}

void JitsiConnector::setNoiseSuppressionEnabled(bool enabled)
{
    if (m_isNoiseSupression != enabled) {
        Q_EMIT executeSetNoiseSupressionCommand(m_isNoiseSupression);
        Q_EMIT isNoiseSuppressionEnabledChanged();
    }
}

void JitsiConnector::setSubtitlesEnabled(bool enabled)
{
    if (m_isSubtitles != enabled) {
        Q_EMIT executeToggleSubtitlesCommand();
        Q_EMIT isSubtitlesEnabledChanged();
    }
}

void JitsiConnector::setSharingScreen(bool shareScreen)
{
    if (m_isSharingScreen != shareScreen) {
        Q_EMIT executeToggleShareScreenCommand();
    }
}

ConferenceParticipant::Role JitsiConnector::ownRole() const
{
    return m_ownRole;
}

void JitsiConnector::kickParticipant(const QString &id)
{
    Q_EMIT executeKickParticipantCommand(id);
}

void JitsiConnector::kickParticipant(ConferenceParticipant *participant)
{
    if (!participant) {
        qCCritical(lcJitsiConnector) << "Cannot kick nullptr participant";
        return;
    }
    kickParticipant(participant->id());
}

void JitsiConnector::grantParticipantRole(const QString &participantId,
                                          ConferenceParticipant::Role newRole)
{
    if (newRole != ConferenceParticipant::Role::Moderator) {
        qCCritical(lcJitsiConnector) << "Role" << newRole << "is not supported by Jitsi Meet";
    }
    Q_EMIT executeGrantModeratorCommand(participantId);
}

void JitsiConnector::grantParticipantRole(ConferenceParticipant *participant,
                                          ConferenceParticipant::Role newRole)
{
    if (!participant) {
        qCCritical(lcJitsiConnector) << "Cannot grant role to nullptr participant";
        return;
    }
    grantParticipantRole(participant->id(), newRole);
}

void JitsiConnector::setLargeVideoParticipant(ConferenceParticipant *participant)
{
    if (m_largeVideoParticipant != participant) {
        m_largeVideoParticipant = participant;
        Q_EMIT largeVideoParticipantChanged();
    }
}

QUrl JitsiConnector::conferenceUrl() const
{
    return QUrl(QString("%1/%2").arg(GlobalInfo::instance().jitsiUrl(), conferenceName()));
}

#undef GONNECT_ASSERT
