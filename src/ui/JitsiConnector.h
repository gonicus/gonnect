#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QPointer>
#include "ICallState.h"
#include "JitsiMediaDevice.h"

class CallHistoryItem;
class SIPAudioDevice;
class Notification;

class JitsiConnector : public ICallState
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isInRoom READ isInRoom NOTIFY isInRoomChanged FINAL)
    Q_PROPERTY(bool isOnHold READ isOnHold NOTIFY isOnHoldChanged FINAL)
    Q_PROPERTY(bool isMuted READ isMuted NOTIFY isMutedChanged FINAL)
    Q_PROPERTY(bool isVideoMuted READ isVideoMuted NOTIFY isVideoMutedChanged FINAL)
    Q_PROPERTY(bool isVideoAvailable READ isVideoAvailable NOTIFY isVideoAvailableChanged FINAL)
    Q_PROPERTY(bool isSharingScreen READ isSharingScreen NOTIFY isSharingScreenChanged FINAL)
    Q_PROPERTY(bool isTileView READ isTileView NOTIFY isTileViewChanged FINAL)
    Q_PROPERTY(bool isHandRaised READ isHandRaised NOTIFY isHandRaisedChanged FINAL)
    Q_PROPERTY(bool isNoiseSupression READ isNoiseSupression NOTIFY isNoiseSupressionChanged FINAL)
    Q_PROPERTY(bool isSubtitles READ isSubtitles NOTIFY isSubtitlesChanged FINAL)
    Q_PROPERTY(
            bool isPasswordRequired READ isPasswordRequired NOTIFY isPasswordRequiredChanged FINAL)
    Q_PROPERTY(bool isPasswordEntryRequired READ isPasswordEntryRequired NOTIFY
                       isPasswordEntryRequiredChanged FINAL)
    Q_PROPERTY(QString roomPassword READ roomPassword NOTIFY roomPasswordChanged FINAL)
    Q_PROPERTY(bool isModerator READ isModerator NOTIFY ownRoleChanged FINAL)
    Q_PROPERTY(uint numberOfParticipants READ numberOfParticipants NOTIFY
                       numberOfParticipantsChanged FINAL)
    Q_PROPERTY(QString largeVideoParticipantId MEMBER m_largeVideoParticipantId NOTIFY
                       largeVideoParticipantIdChanged FINAL)
    Q_PROPERTY(QString ownJitsiId READ jitsiId NOTIFY jitsiIdChanged FINAL)

    Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged FINAL)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged FINAL)
    Q_PROPERTY(QUrl roomUrl READ roomUrl NOTIFY roomNameChanged FINAL)

    Q_PROPERTY(QString currentAudioInputDeviceId READ currentAudioInputDeviceId NOTIFY
                       currentAudioInputDeviceChanged FINAL)
    Q_PROPERTY(QString currentAudioOutputDeviceId READ currentAudioOutputDeviceId NOTIFY
                       currentAudioOutputDeviceChanged FINAL)
    Q_PROPERTY(QString currentVideoInputDeviceId READ currentVideoInputDeviceId NOTIFY
                       currentVideoInputDeviceChanged FINAL)

    Q_PROPERTY(JitsiConnector::VideoQuality videoQuality READ videoQuality NOTIFY
                       videoQualityChanged FINAL)

public:
    enum class VideoQuality { AudioOnly = 0, Low = 180, Standard = 360, Highest = 2160 };
    Q_ENUM(VideoQuality)

    enum class MeetingStartFlag {
        AudioActive = 1 << 0,
        VideoActive = 1 << 1,
        ScreenShareActive = 1 << 2
    };
    Q_ENUM(MeetingStartFlag)
    Q_DECLARE_FLAGS(MeetingStartFlags, MeetingStartFlag)
    Q_FLAG(MeetingStartFlags)

    struct Message
    {
        QString fromId;
        QString nickName;
        QString message;
        QDateTime timestamp;
        bool isPrivateMessage = false;
        bool isSystemMessage = false;
    };

    enum class ParticipantRole { None, Participant, Moderator };
    Q_ENUM(ParticipantRole)

    struct Participant
    {
        QString id;
        QString displayName;
        ParticipantRole role = ParticipantRole::None;
    };

    static QString participantRoleToString(const ParticipantRole role);

    bool isModerator() const { return m_ownRole == ParticipantRole::Moderator; }

    explicit JitsiConnector(QObject *parent = nullptr);

    QString jitsiId() const { return m_jitsiId; }
    Q_INVOKABLE void setJitsiId(QString id);

    Q_INVOKABLE void
    setCallHistoryItem(QPointer<CallHistoryItem> callHistoryItem = QPointer<CallHistoryItem>());

    virtual ContactInfo remoteContactInfo() const override;

    // Should be called by the JS code once when all initialization is done
    Q_INVOKABLE void apiLoadingFinished();

    // The time and date when the current meeting was joined. Only valid when isInRoom() is true.
    Q_INVOKABLE QDateTime establishedDateTime() const { return m_establishedDateTime; };

    Q_INVOKABLE void addError(QString type, QString name, QString message = "",
                              bool isFatal = false, QVariantMap details = QVariantMap());

    Q_INVOKABLE void addIncomingMessage(QString fromId, QString nickName, QString message,
                                        QDateTime stamp, bool isPrivateMessage);

    Q_INVOKABLE QString jitsiHtml();
    Q_INVOKABLE QString jitsiJavascript();

    Q_INVOKABLE void enterRoom(const QString &roomName, const QString &displayName,
                               JitsiConnector::MeetingStartFlags startFlags);
    Q_INVOKABLE void leaveRoom();
    Q_INVOKABLE void terminateRoom();
    QString roomName() const { return m_roomName; }
    QString displayName() const { return m_displayName; }
    QUrl roomUrl();
    bool isInRoom() const { return m_isInRoom; }

    Q_INVOKABLE void sendMessage(QString message);
    const QList<Message> &messages() const { return m_messages; }

    bool isOnHold() const { return m_isOnHold; }

    // Toggles the Jitsi-internal mute. For globally setting the mute state, use GlobalMuteState
    // instead.
    void toggleMute();
    bool isMuted() const { return m_isMuted; }
    // Only sets the variable and is used by the jitsi api. To actually change the muted state, use
    // toggleMute().
    Q_INVOKABLE void setIsMuted(bool value);

    // Only sets the variable by the jitsi api. Does not switch any availablility.
    Q_INVOKABLE void setIsVideoAvailable(bool value);
    bool isVideoAvailable() const { return m_isVideoAvailable; }

    Q_INVOKABLE void toggleVideoMute();
    bool isVideoMuted() const { return m_isVideoMuted; };
    // Only sets the variable and is used by the jitsi api. To actually change the muted state, use
    // toggleVideoMute().
    Q_INVOKABLE void setIsVideoMuted(bool value);

    Q_INVOKABLE void toggleVirtualBackgroundDialog();

    Q_INVOKABLE void toggleScreenShare();
    bool isSharingScreen() const { return m_isSharingScreen; }
    // Only sets the variable and is used by the jitsi api. To actually change the screenShare
    // state, use toggleScreenShare().
    Q_INVOKABLE void setIsSharingScreen(bool value);

    Q_INVOKABLE void toggleTileView();
    bool isTileView() const { return m_isTileView; }
    // Only sets the variable and is used by the jitsi api. To actually change the isTileView state,
    // use toggleTileView().
    Q_INVOKABLE void setIsTileView(bool value);

    Q_INVOKABLE void toggleRaiseHand();
    bool isHandRaised() const { return m_isHandRaised; }

    Q_INVOKABLE void toggleNoiseSupression();
    bool isNoiseSupression() const { return m_isNoiseSupression; }

    Q_INVOKABLE void toggleSubtitles();
    bool isSubtitles() const { return m_isSubtitles; }

    /// Whether the room is protected by a password.
    bool isPasswordRequired() const { return m_isPasswordRequired; }
    /// Whether this client must enter a password in order to enter the room.
    bool isPasswordEntryRequired() const { return m_isPasswordEntryRequired; }
    /// The password required to enter the room. Only set if this client has set the password, empty
    /// otherwise.
    QString roomPassword() const { return m_roomPassword; }
    /// Must be called with the password if isPasswordEntryRequired() is true.
    Q_INVOKABLE void passwordEntered(const QString &password, bool shouldRemember);
    /// Sets a password for the room. Only possible when being a moderator.
    Q_INVOKABLE void setRoomPassword(QString value);
    // For use by jitsi api only
    Q_INVOKABLE void onPasswordRequired();

    VideoQuality videoQuality() const { return m_videoQuality; }
    Q_INVOKABLE void setVideoQuality(JitsiConnector::VideoQuality quality);
    // For use by jitsi api only
    Q_INVOKABLE void setVideoQualityInternal(uint quality);

    const QList<JitsiConnector::Participant> &participants() const { return m_participants; }
    // For use by jitsi api only
    Q_INVOKABLE void addParticipant(const QString &id, const QString &displayName);
    Q_INVOKABLE void removeParticipant(const QString &id);
    Q_INVOKABLE void setParticipantRole(const QString &id, const QString &roleString);
    uint numberOfParticipants() const { return m_participants.size(); }

    Q_INVOKABLE void kickParticipant(const QString &id);
    Q_INVOKABLE void grantParticipantModerator(const QString &id);
    Q_INVOKABLE void muteAll();

    // For setting device info from the jitsi api. Does not change devices in jitsi.
    Q_INVOKABLE void setJitsiDevices(const QVariantMap availableDevices,
                                     const QVariantMap currentDevices);
    QList<JitsiMediaDevice *> audioInputDevices() const { return m_audioInputDevices; }
    QList<JitsiMediaDevice *> audioOutputDevices() const { return m_audioOutputDevices; }
    QList<JitsiMediaDevice *> videoInputDevices() const { return m_videoInputDevices; }

    QString currentAudioInputDeviceId() const
    {
        return m_currentAudioInputDevice ? m_currentAudioInputDevice->deviceId() : "";
    }
    QString currentAudioOutputDeviceId() const
    {
        return m_currentAudioOutputDevice ? m_currentAudioOutputDevice->deviceId() : "";
    }
    QString currentVideoInputDeviceId() const
    {
        return m_currentVideoInputDevice ? m_currentVideoInputDevice->deviceId() : "";
    }

    Q_INVOKABLE void selectAudioInputDevice(const QString &deviceId);
    Q_INVOKABLE void selectAudioOutputDevice(const QString &deviceId);
    Q_INVOKABLE void selectVideoInputDevice(const QString &deviceId);

protected:
    virtual void toggleHoldImpl() override;

private slots:
    void onHeadsetHookSwitchChanged();
    void transferAudioManagerDevicesToJitsi();
    void transferVideoManagerDeviceToJitsi();

private:
    void setRoomName(const QString &name);
    void setDisplayName(const QString &name);
    void setIsHandRaised(bool value);
    void setIsInRoom(bool value);
    void setIsOnHold(bool value);
    void setIsPasswordRequired(bool value);
    void setIsPasswordEntryRequired(bool value);
    void setCurrentAudioInputDevice(JitsiMediaDevice *device);
    void setCurrentAudioOutputDevice(JitsiMediaDevice *device);
    void setCurrentVideoInputDevice(JitsiMediaDevice *device);
    void updateVideoCallState();
    JitsiMediaDevice *createJitsiMediaDevice(const QVariantMap &deviceMap);
    JitsiMediaDevice *findDevice(const QList<JitsiMediaDevice *> &deviceList,
                                 const QString &deviceId) const;
    SIPAudioDevice *jitsiToSipDevice(const JitsiMediaDevice *jitsiDevice) const;
    JitsiMediaDevice *sipToJitsiDevice(const SIPAudioDevice *sipDevice) const;
    void addRoomMessage(QString message, QDateTime stamp = QDateTime::currentDateTime());
    QString jitsiDisplayName() const;

    ParticipantRole m_ownRole = ParticipantRole::None;
    QString m_jitsiId;
    QString m_muteTag;
    QString m_largeVideoParticipantId;
    bool m_isInRoom = false;
    bool m_isMuted = false;
    bool m_isVideoMuted = false;
    bool m_isVideoAvailable = false;
    bool m_isSharingScreen = false;
    bool m_isTileView = false;
    bool m_isHandRaised = false;
    bool m_startWithVideo = false;
    bool m_isOnHold = false;
    bool m_wasVideoMutedBeforeHold = false;
    bool m_isNoiseSupression = false;
    bool m_isSubtitles = false;
    bool m_passwordAlreadyRequested = false;
    bool m_isPasswordRequired = false;
    bool m_isPasswordEntryRequired = false;
    bool m_isApiLoadingFinished = false;
    bool m_isToggleScreenSharePending = false;
    QString m_roomPassword;
    VideoQuality m_videoQuality = VideoQuality::Standard;

    QString m_roomName;
    QString m_displayName;
    QList<Message> m_messages;
    QList<Participant> m_participants;
    QPointer<CallHistoryItem> m_callHistoryItem;
    QDateTime m_establishedDateTime;
    QList<Notification *> m_chatNotifications;
    Notification *m_inConferenceNotification = nullptr;

    QList<JitsiMediaDevice *> m_audioInputDevices;
    QList<JitsiMediaDevice *> m_audioOutputDevices;
    QList<JitsiMediaDevice *> m_videoInputDevices;

    JitsiMediaDevice *m_currentAudioInputDevice = nullptr;
    JitsiMediaDevice *m_currentAudioOutputDevice = nullptr;
    JitsiMediaDevice *m_currentVideoInputDevice = nullptr;

signals:
    void jitsiIdChanged();
    void roomNameChanged();
    void displayNameChanged();
    void ownRoleChanged();
    void roomPasswordChanged();
    void videoQualityChanged();

    void beganJitsiDevicesReset();
    void endedJitsiDevicesReset();

    void currentAudioInputDeviceChanged();
    void currentAudioOutputDeviceChanged();
    void currentVideoInputDeviceChanged();

    void messageSent(QString message);
    void messageAdded(qsizetype index);
    void messagesReset();

    void participantAdded(qsizetype index, const QString id);
    void participantRemoved(qsizetype index, const QString id);
    void participantRoleChanged(qsizetype index, const QString id,
                                const JitsiConnector::ParticipantRole role);
    void participantsCleared();
    void numberOfParticipantsChanged();
    void largeVideoParticipantIdChanged();

    void executeLeaveRoomCommand();
    void executeEndConferenceCommand();
    void executePasswordCommand(QString password);
    void executeToggleAudioCommand();
    void executeToggleVideoCommand();
    void executeToggleVirtualBackgroundDialogCommand();
    void executeToggleShareScreenCommand();
    void executeToggleTileViewCommand();
    void executeToggleRaiseHandCommand();
    void executeToggleSubtitlesCommand();
    void executeSetAudioInputDeviceCommand(QString devicId);
    void executeSetAudioOutputDeviceCommand(QString devicId);
    void executeSetVideoInputDeviceCommand(QString devicId);
    void executeSetNoiseSupressionCommand(bool value);
    void executeSetLargeVideoParticipant(QString id);
    void executeKickParticipantCommand(QString id);
    void executeGrantModeratorCommand(QString id);
    void executeMuteAllCommand();
    void executeSetVideoQualityCommand(uint videoQuality);
    void executeSetAudioOnlyCommand(bool isAudioOnly);

    void isInRoomChanged();
    void isMutedChanged();
    void isOnHoldChanged();
    void isVideoAvailableChanged();
    void isVideoMutedChanged();
    void isSharingScreenChanged();
    void isTileViewChanged();
    void isHandRaisedChanged();
    void isNoiseSupressionChanged();
    void isSubtitlesChanged();
    void isPasswordRequiredChanged();
    void isPasswordEntryRequiredChanged();
};
