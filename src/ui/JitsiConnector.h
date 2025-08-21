#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QPointer>
#include "IConferenceConnector.h"
#include "JitsiMediaDevice.h"
#include "ConferenceChatMessage.h"
#include "ConferenceParticipant.h"

class CallHistoryItem;
class SIPAudioDevice;
class Notification;

class JitsiConnector : public IConferenceConnector
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isPasswordEntryRequired READ isPasswordEntryRequired NOTIFY
                       isPasswordEntryRequiredChanged FINAL)
    Q_PROPERTY(QString roomPassword READ roomPassword NOTIFY roomPasswordChanged FINAL)
    Q_PROPERTY(bool isModerator READ isModerator NOTIFY ownRoleChanged FINAL)
    Q_PROPERTY(uint numberOfParticipants READ numberOfParticipants NOTIFY
                       numberOfParticipantsChanged FINAL)
    Q_PROPERTY(QString largeVideoParticipantId MEMBER m_largeVideoParticipantId NOTIFY
                       largeVideoParticipantIdChanged FINAL)
    Q_PROPERTY(QString currentAudioInputDeviceId READ currentAudioInputDeviceId NOTIFY
                       currentAudioInputDeviceChanged FINAL)
    Q_PROPERTY(QString currentAudioOutputDeviceId READ currentAudioOutputDeviceId NOTIFY
                       currentAudioOutputDeviceChanged FINAL)
    Q_PROPERTY(QString currentVideoInputDeviceId READ currentVideoInputDeviceId NOTIFY
                       currentVideoInputDeviceChanged FINAL)

public:
    static QString participantRoleToString(const ConferenceParticipant::Role role);

    bool isModerator() const { return m_ownRole == ConferenceParticipant::Role::Moderator; }

    explicit JitsiConnector(QObject *parent = nullptr);

    Q_INVOKABLE void setJitsiId(QString id);

    Q_INVOKABLE void
    setCallHistoryItem(QPointer<CallHistoryItem> callHistoryItem = QPointer<CallHistoryItem>());

    virtual ContactInfo remoteContactInfo() const override;

    // The time and date when the current meeting was joined. Only valid when isInConference() is
    // true.
    Q_INVOKABLE QDateTime establishedDateTime() const { return m_establishedDateTime; };

    Q_INVOKABLE void addError(QString type, QString name, QString message = "",
                              bool isFatal = false, QVariantMap details = QVariantMap());

    Q_INVOKABLE void addIncomingMessage(QString fromId, QString nickName, QString message,
                                        QDateTime stamp, bool isPrivateMessage);

    Q_INVOKABLE void enterRoom(const QString &roomName, const QString &displayName,
                               IConferenceConnector::StartFlags startFlags);

    Q_INVOKABLE virtual ConferenceChatMessage *sendMessage(const QString &message) override;

    // Toggles the Jitsi-internal mute. For globally setting the mute state, use GlobalMuteState
    // instead.
    void toggleMute();

    Q_INVOKABLE void toggleVirtualBackgroundDialog();

    /// Whether this client must enter a password in order to enter the room.
    bool isPasswordEntryRequired() const { return m_isPasswordEntryRequired; }
    /// The password required to enter the room. Only set if this client has set the password, empty
    /// otherwise.
    QString roomPassword() const { return m_roomPassword; }
    /// Sets a password for the room. Only possible when being a moderator.
    Q_INVOKABLE void setRoomPassword(QString value);

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

    // INTERNAL API
    // These methods are meant to be called by the JS code

    Q_INVOKABLE QString jitsiHtmlInternal();
    Q_INVOKABLE QString jitsiJavascriptInternal();

    // Should be called by the JS code once when all initialization is done
    Q_INVOKABLE void apiLoadingFinishedInternal();

    Q_INVOKABLE void setVideoAvailableInternal(bool value);
    Q_INVOKABLE void setVideoQualityInternal(uint quality);
    Q_INVOKABLE void setIsSharingScreenInternal(bool value);
    Q_INVOKABLE void setIsTileViewInternal(bool value);

    Q_INVOKABLE void onPasswordRequired();

    Q_INVOKABLE void addParticipant(const QString &id, const QString &displayName);
    Q_INVOKABLE void removeParticipant(const QString &id);
    Q_INVOKABLE void setParticipantRole(const QString &id, const QString &roleString);
    uint numberOfParticipants() const { return m_participants.size(); }

    // Interface methods

    virtual bool hasCapability(const Capability capabilityToCheck) const override;
    virtual bool isInitialized() override { return m_isApiLoadingFinished; }
    virtual bool isInConference() const override { return m_isInConference; }
    virtual bool isPasswordRequired() const override { return m_isPasswordRequired; }
    virtual void joinConference(const QString &conferenceId) override;
    virtual void enterPassword(const QString &password, bool rememberPassword) override;
    virtual void leaveConference() override;
    virtual void terminateConference() override;
    inline virtual bool isAudioAvailable() const override { return true; }
    virtual bool isOnHold() const override { return m_isOnHold; }
    virtual void setOnHold(bool shallHold) override;
    virtual void setAudioMuted(bool shallMute) override;
    virtual bool isAudioMuted() const override { return m_isAudioMuted; }
    virtual bool isVideoAvailable() const override { return m_isVideoAvailable; };
    virtual bool isVideoMuted() const override { return m_isVideoMuted; }
    virtual void setVideoMuted(bool shallMute) override;
    virtual void setHandRaised(bool handRaised) override;
    virtual void setTileView(bool showTileView) override;
    virtual bool isHandRaised() const override { return m_isHandRaised; }
    virtual bool isTileView() const override { return m_isTileView; }
    virtual void setNoiseSuppressionEnabled(bool enabled) override;
    virtual bool isNoiseSuppressionEnabled() const override { return m_isNoiseSupression; }
    virtual void setSubtitlesEnabled(bool enabled) override;
    virtual bool isSubtitlesEnabled() const override { return m_isSubtitles; }
    virtual QList<ConferenceChatMessage *> messages() const override { return m_messages; }
    virtual QString ownId() const override { return m_jitsiId; }
    virtual ConferenceParticipant::Role ownRole() const override;
    virtual QList<ConferenceParticipant *> participants() const override { return m_participants; }
    virtual void kickParticipant(const QString &id) override;
    virtual void kickParticipant(ConferenceParticipant *participant) override;
    virtual void grantParticipantRole(const QString &participantId,
                                      ConferenceParticipant::Role newRole) override;
    virtual void grantParticipantRole(ConferenceParticipant *participant,
                                      ConferenceParticipant::Role newRole) override;
    virtual void muteAll() override;

    virtual QString displayName() const override { return m_displayName; }

    virtual QString conferenceName() const override { return m_roomName; }
    virtual QUrl conferenceUrl() const override;
    virtual void setDisplayName(const QString &displayName) override;
    virtual void setSharingScreen(bool shareScreen) override;
    virtual bool isSharingScreen() const override { return m_isSharingScreen; }
    virtual void setVideoQuality(VideoQuality) override;
    virtual VideoQuality videoQuality() const override { return m_videoQuality; }

protected:
    virtual void toggleHoldImpl() override;

private slots:
    void onHeadsetHookSwitchChanged();
    void transferAudioManagerDevicesToJitsi();
    void transferVideoManagerDeviceToJitsi();

private:
    void setConferenceName(const QString &name);
    void setIsInConference(bool value);
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

    ConferenceParticipant::Role m_ownRole = ConferenceParticipant::Role::None;
    QString m_jitsiId;
    QString m_muteTag;
    QString m_largeVideoParticipantId;
    bool m_isInConference = false;
    bool m_isAudioMuted = false;
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
    VideoQuality m_videoQuality = VideoQuality::Average;

    QString m_roomName;
    QString m_displayName;
    QList<ConferenceChatMessage *> m_messages;
    QList<ConferenceParticipant *> m_participants;
    QPointer<CallHistoryItem> m_callHistoryItem;
    QDateTime m_establishedDateTime;
    QList<Notification *> m_chatNotifications;

    QList<JitsiMediaDevice *> m_audioInputDevices;
    QList<JitsiMediaDevice *> m_audioOutputDevices;
    QList<JitsiMediaDevice *> m_videoInputDevices;

    JitsiMediaDevice *m_currentAudioInputDevice = nullptr;
    JitsiMediaDevice *m_currentAudioOutputDevice = nullptr;
    JitsiMediaDevice *m_currentVideoInputDevice = nullptr;

signals:
    void jitsiIdChanged();
    void roomPasswordChanged();

    void beganJitsiDevicesReset();
    void endedJitsiDevicesReset();

    void currentAudioInputDeviceChanged();
    void currentAudioOutputDeviceChanged();
    void currentVideoInputDeviceChanged();

    void messageSent(QString message);
    void messageAdded(qsizetype index);
    void messagesReset();

    void participantRoleChanged(qsizetype index, const QString id,
                                const ConferenceParticipant::Role role);
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

    void isPasswordEntryRequiredChanged();
};
