#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QPointer>
#include "ConferenceChatRoom.h"
#include "IConferenceConnector.h"
#include "JitsiMediaDevice.h"
#include "ConferenceUser.h"
#include "MuteSyncGuard.h"

class CallHistoryItem;
class SIPAudioDevice;
class Notification;

class JitsiConnector : public IConferenceConnector
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit JitsiConnector(QObject *parent = nullptr);
    ~JitsiConnector();

    QString ownDisplayName() override;

    // INTERNAL API
    // These methods are meant to be called by the JS code

    Q_INVOKABLE void setJitsiId(QString id);
    Q_INVOKABLE void addError(QString type, QString name, QString message = "",
                              bool isFatal = false, QVariantMap details = QVariantMap());
    Q_INVOKABLE void addIncomingMessage(QString fromId, QString nickName, QString message,
                                        QDateTime stamp, bool isPrivateMessage);
    Q_INVOKABLE void setJitsiDevices(const QVariantMap availableDevices);

    Q_INVOKABLE QString jitsiHtmlInternal();
    Q_INVOKABLE QString jitsiJavascriptInternal();

    // Should be called by the JS code once when all initialization is done
    Q_INVOKABLE void apiLoadingFinishedInternal();

    Q_INVOKABLE void setVideoMutedInternal(bool value);
    Q_INVOKABLE void setVideoAvailableInternal(bool value);
    Q_INVOKABLE void setVideoQualityInternal(uint quality);
    Q_INVOKABLE void setIsSharingScreenInternal(bool value);
    Q_INVOKABLE void setIsTileViewInternal(bool value);

    Q_INVOKABLE void onPasswordRequired();

    Q_INVOKABLE void addUser(const QString &id, const QString &displayName);
    Q_INVOKABLE void removeUser(const QString &id);
    Q_INVOKABLE void setUserRole(const QString &id, const QString &roleString);
    Q_INVOKABLE void setLargeVideoUserById(const QString &id);

    // Interface methods
    ContactInfo remoteContactInfo() const override;

    bool hasCapability(const Capability capabilityToCheck) const override;
    bool isInitialized() override { return m_isApiLoadingFinished; }
    bool isInConference() const override { return m_isInConference; }
    bool hasWhiteboard() const override { return m_hasWhiteboard; }
    void toggleWhiteboard() override;
    bool hasTextpad() const override { return m_hasTextpad; }
    bool isPasswordRequired() const override { return m_isPasswordRequired; }
    QString roomPassword() const override { return m_roomPassword; }
    void setRoomPassword(QString value) override;
    void joinConference(const QString &conferenceId, const QString &displayName,
                        IConferenceConnector::StartFlags startFlags) override;
    void enterPassword(const QString &password, bool rememberPassword) override;
    void leaveConference() override;
    void terminateConference() override;
    QDateTime establishedDateTime() const override { return m_establishedDateTime; };
    inline bool isAudioAvailable() const override { return true; }
    bool isOnHold() const override { return m_isOnHold; }
    void setOnHold(bool shallHold) override;
    void setAudioMuted(bool shallMute) override;
    bool isAudioMuted() const override { return m_isAudioMuted; }
    bool isVideoAvailable() const override { return m_isVideoAvailable; };
    bool isVideoMuted() const override { return m_isVideoMuted; }
    void setVideoMuted(bool shallMute) override;
    void setHandRaised(bool handRaised) override;
    void setTileView(bool showTileView) override;
    bool hasDialIn() const override;
    void requestDialInInfo() override;
    bool isHandRaised() const override { return m_isHandRaised; }
    bool isTileView() const override { return m_isTileView; }
    void setNoiseSuppressionEnabled(bool enabled) override;
    bool isNoiseSuppressionEnabled() const override { return m_isNoiseSupression; }
    void setSubtitlesEnabled(bool enabled) override;
    bool isSubtitlesEnabled() const override { return m_isSubtitles; }
    IChatRoom *chatRoom() override { return m_chatRoom; }
    QString ownId() const override { return m_jitsiId; }
    ConferenceUser::Role ownRole() const override;
    QList<ConferenceUser *> users() const override { return m_users; }
    uint numberOfUsers() const override { return m_users.size(); }
    void kickUser(const QString &id) override;
    void kickUser(ConferenceUser *user) override;
    void grantUserRole(const QString &userId, ConferenceUser::Role newRole) override;
    void grantUserRole(ConferenceUser *user, ConferenceUser::Role newRole) override;
    ConferenceUser *largeVideoUser() const override { return m_largeVideoUser; };
    void setLargeVideoUser(ConferenceUser *user) override;
    void muteAll() override;
    void setCallHistoryItem(
            QPointer<CallHistoryItem> callHistoryItem = QPointer<CallHistoryItem>()) override;
    QString displayName() const override { return m_displayName; }
    QString conferenceName() const override { return m_roomName; }
    QUrl conferenceUrl() const override;
    void setDisplayName(const QString &displayName) override;
    void setSharingScreen(bool shareScreen) override;
    bool isSharingScreen() const override { return m_isSharingScreen; }
    void setVideoQuality(VideoQuality) override;
    VideoQuality videoQuality() const override { return m_videoQuality; }
    void showVirtualBackgroundDialog() override;

protected:
    void toggleHoldImpl() override;

private Q_SLOTS:
    void onHeadsetHookSwitchChanged();
    void transferAudioManagerDevicesToJitsi();
    void transferVideoManagerDeviceToJitsi();

private:
    // Toggles the Jitsi-internal mute. For globally setting the mute state, use GlobalMuteState
    // instead.
    void toggleMute();

    void checkJitsiBackendFeatures();
    void checkMeetingEstablished();

    void setHasWhiteboard(bool value);
    void setHasTextpad(bool value);
    void setConferenceName(const QString &name);
    void setIsInConference(bool value);
    void setIsOnHold(bool value);
    void setIsPasswordRequired(bool value);
    void selectAudioInputDevice(const QString &deviceId);
    void selectAudioOutputDevice(const QString &deviceId);
    void selectVideoInputDevice(const QString &deviceId);
    void updateVideoCallState();
    JitsiMediaDevice *createJitsiMediaDevice(const QVariantMap &deviceMap);
    JitsiMediaDevice *findDevice(const QList<JitsiMediaDevice *> &deviceList,
                                 const QString &deviceId) const;
    SIPAudioDevice *jitsiToSipDevice(const JitsiMediaDevice *jitsiDevice) const;
    JitsiMediaDevice *sipToJitsiDevice(const SIPAudioDevice *sipDevice) const;
    void addRoomMessage(QString message, QDateTime stamp = QDateTime::currentDateTime());
    QString jitsiDisplayName() const;

    ConferenceUser::Role m_ownRole = ConferenceUser::Role::None;
    QString m_jitsiId;
    MuteSyncGuard m_muteSync;
    bool m_didExecuteAudioMuteToggle = false;
    ConferenceUser *m_largeVideoUser = nullptr;
    ConferenceChatRoom *m_chatRoom = nullptr;
    bool m_isInConference = false;
    bool m_meetingEstablishedEmitted = false;
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
    bool m_isApiLoadingFinished = false;
    bool m_isToggleScreenSharePending = false;
    bool m_hasWhiteboard = false;
    bool m_hasTextpad = false;
    QUrl m_dialInConfCodeUrl;
    QUrl m_dialInNumbersUrl;
    QString m_roomPassword;
    VideoQuality m_videoQuality = VideoQuality::Average;

    QString m_roomName;
    QString m_displayName;
    QList<ConferenceUser *> m_users;
    QPointer<CallHistoryItem> m_callHistoryItem;
    QDateTime m_establishedDateTime;
    QList<Notification *> m_chatNotifications;
    Notification *m_inConferenceNotification = nullptr;

    QList<JitsiMediaDevice *> m_audioInputDevices;
    QList<JitsiMediaDevice *> m_audioOutputDevices;
    QList<JitsiMediaDevice *> m_videoInputDevices;

Q_SIGNALS:
    // Internal API
    // These events are meant to be received by JS code.
    void messageSent(QString message);

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
    void executeToggleWhiteboardCommand();
    void executeSetAudioInputDeviceCommand(QString devicId);
    void executeSetAudioOutputDeviceCommand(QString devicId);
    void executeSetVideoInputDeviceCommand(QString devicId);
    void executeSetNoiseSupressionCommand(bool value);
    void executeSetLargeVideoUser(QString id);
    void executeKickUserCommand(QString id);
    void executeGrantModeratorCommand(QString id);
    void executeMuteAllCommand();
    void executeSetVideoQualityCommand(uint videoQuality);
    void executeSetAudioOnlyCommand(bool isAudioOnly);
};
