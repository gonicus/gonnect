#pragma once

#include "ICallState.h"
#include "IChatRoom.h"
#include "ConferenceParticipant.h"
#include "CallHistoryItem.h"

#include <QPointer>

class ChatMessage;

class IConferenceConnector : public ICallState
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool isInitialized READ isInitialized NOTIFY isInitializedChanged FINAL)
    Q_PROPERTY(bool isInConference READ isInConference NOTIFY isInConferenceChanged FINAL)
    Q_PROPERTY(
            bool isPasswordRequired READ isPasswordRequired NOTIFY isPasswordRequiredChanged FINAL)
    Q_PROPERTY(QString roomPassword READ roomPassword NOTIFY roomPasswordChanged FINAL)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged FINAL)
    Q_PROPERTY(QString conferenceName READ conferenceName NOTIFY conferenceNameChanged FINAL)
    Q_PROPERTY(QUrl conferenceUrl READ conferenceUrl NOTIFY conferenceUrlChanged FINAL)
    Q_PROPERTY(bool isOnHold READ isOnHold NOTIFY isOnHoldChanged FINAL)
    Q_PROPERTY(bool isAudioAvailable READ isAudioAvailable NOTIFY isAudioAvailableChanged FINAL)
    Q_PROPERTY(bool isVideoAvailable READ isVideoAvailable NOTIFY isVideoAvailableChanged FINAL)
    Q_PROPERTY(bool isAudioMuted READ isAudioMuted NOTIFY isAudioMutedChanged FINAL)
    Q_PROPERTY(bool isVideoMuted READ isVideoMuted NOTIFY isVideoMutedChanged FINAL)
    Q_PROPERTY(bool isHandRaised READ isHandRaised NOTIFY isHandRaisedChanged FINAL)
    Q_PROPERTY(bool isTileView READ isTileView NOTIFY isTileViewChanged FINAL)
    Q_PROPERTY(bool isSharingScreen READ isSharingScreen NOTIFY isSharingScreenChanged FINAL)
    Q_PROPERTY(bool isNoiseSuppressionEnabled READ isNoiseSuppressionEnabled NOTIFY
                       isNoiseSuppressionEnabledChanged FINAL)
    Q_PROPERTY(
            bool isSubtitlesEnabled READ isSubtitlesEnabled NOTIFY isSubtitlesEnabledChanged FINAL)
    Q_PROPERTY(IConferenceConnector::VideoQuality videoQuality READ videoQuality NOTIFY
                       videoQualityChanged FINAL)
    Q_PROPERTY(QString ownId READ ownId NOTIFY ownIdChanged FINAL)
    Q_PROPERTY(ConferenceParticipant::Role ownRole READ ownRole NOTIFY ownRoleChanged FINAL)
    Q_PROPERTY(uint numberOfParticipants READ numberOfParticipants NOTIFY
                       numberOfParticipantsChanged FINAL)
    Q_PROPERTY(ConferenceParticipant *largeVideoParticipant READ largeVideoParticipant NOTIFY
                       largeVideoParticipantChanged FINAL)

public:
    IConferenceConnector(QObject *parent = nullptr) : ICallState{ parent } { }

    enum class Capability {
        AudioMute = 1,
        ChatInCall,
        Holdable,
        RaiseHand,
        ScreenShare,
        Sharable,
        Subtitles,
        MuteAll,
        NoiseSuppression,
        ParticipantRoles,
        ParticipantKickable,
        RoomPassword,
        ShareUrl,
        TileView,
        VideoMute,
        VideoQualityAdjustable,
    };
    Q_ENUM(Capability)

    Q_INVOKABLE virtual bool
    hasCapability(const IConferenceConnector::Capability capabilityToCheck) const = 0;

    enum class StartFlag { AudioActive = 1 << 0, VideoActive = 1 << 1, ScreenShareActive = 1 << 2 };
    Q_ENUM(StartFlag)
    Q_DECLARE_FLAGS(StartFlags, StartFlag)
    Q_FLAG(StartFlags)

    virtual bool isInitialized() = 0;
    virtual bool isInConference() const = 0;
    virtual bool isPasswordRequired() const = 0;
    virtual QString roomPassword() const = 0;
    Q_INVOKABLE virtual void setRoomPassword(QString password) = 0;
    Q_INVOKABLE virtual void joinConference(const QString &conferenceId, const QString &displayName,
                                            IConferenceConnector::StartFlags startFlags) = 0;
    Q_INVOKABLE virtual void enterPassword(const QString &password, bool rememberPassword) = 0;
    Q_INVOKABLE virtual void leaveConference() = 0;
    Q_INVOKABLE virtual void terminateConference() = 0;
    Q_INVOKABLE virtual QDateTime establishedDateTime() const = 0;

    Q_INVOKABLE virtual void
    setCallHistoryItem(QPointer<CallHistoryItem> callHistoryItem = QPointer<CallHistoryItem>()) = 0;

    Q_INVOKABLE virtual void setDisplayName(const QString &displayName) = 0;
    virtual QString displayName() const = 0;
    virtual QString conferenceName() const = 0;
    virtual QUrl conferenceUrl() const = 0;

    enum class VideoQuality { AudioOnly, Minimum, Low, Average, High, Maximum };
    Q_ENUM(VideoQuality)

    Q_INVOKABLE virtual void setVideoQuality(IConferenceConnector::VideoQuality) = 0;
    virtual VideoQuality videoQuality() const = 0;

    virtual bool isAudioAvailable() const = 0;
    virtual bool isVideoAvailable() const = 0;

    Q_INVOKABLE virtual void setOnHold(bool shallHold) = 0;
    virtual bool isOnHold() const = 0;

    Q_INVOKABLE virtual void setAudioMuted(bool shallMute) = 0;
    virtual bool isAudioMuted() const = 0;

    Q_INVOKABLE virtual void setVideoMuted(bool shallMute) = 0;
    virtual bool isVideoMuted() const = 0;

    Q_INVOKABLE virtual void setHandRaised(bool handRaised) = 0;
    virtual bool isHandRaised() const = 0;

    Q_INVOKABLE virtual void setTileView(bool showTileView) = 0;
    virtual bool isTileView() const = 0;

    Q_INVOKABLE virtual void setSharingScreen(bool shareScreen) = 0;
    virtual bool isSharingScreen() const = 0;

    Q_INVOKABLE virtual void setNoiseSuppressionEnabled(bool enabled) = 0;
    virtual bool isNoiseSuppressionEnabled() const = 0;

    Q_INVOKABLE virtual void setSubtitlesEnabled(bool enabled) = 0;
    virtual bool isSubtitlesEnabled() const = 0;

    Q_INVOKABLE virtual IChatRoom *chatRoom() = 0;

    virtual QString ownId() const = 0;
    virtual ConferenceParticipant::Role ownRole() const = 0;

    virtual QList<ConferenceParticipant *> participants() const = 0;
    virtual uint numberOfParticipants() const = 0;
    Q_INVOKABLE virtual void kickParticipant(const QString &participantId) = 0;
    virtual void kickParticipant(ConferenceParticipant *participant) = 0;
    Q_INVOKABLE virtual void grantParticipantRole(const QString &participantId,
                                                  ConferenceParticipant::Role newRole) = 0;
    virtual void grantParticipantRole(ConferenceParticipant *participant,
                                      ConferenceParticipant::Role newRole) = 0;
    virtual ConferenceParticipant *largeVideoParticipant() const = 0;
    virtual void setLargeVideoParticipant(ConferenceParticipant *participant) = 0;

    Q_INVOKABLE virtual void muteAll() = 0;
    Q_INVOKABLE virtual void showVirtualBackgroundDialog() = 0;

signals:
    void isInitializedChanged();
    void isInConferenceChanged();
    void isPasswordRequiredChanged();
    void roomPasswordChanged();
    void displayNameChanged();
    void conferenceNameChanged();
    void conferenceUrlChanged();
    void isOnHoldChanged();
    void isAudioAvailableChanged();
    void isVideoAvailableChanged();
    void isAudioMutedChanged();
    void isVideoMutedChanged();
    void isHandRaisedChanged();
    void isTileViewChanged();
    void isSharingScreenChanged();
    void isNoiseSuppressionEnabledChanged();
    void isSubtitlesEnabledChanged();
    void videoQualityChanged();
    void ownIdChanged();
    void ownRoleChanged();
    void participantAdded(qsizetype index, ConferenceParticipant *participant);
    void participantRemoved(qsizetype index, ConferenceParticipant *participant);
    void participantRoleChanged(qsizetype index, ConferenceParticipant *participant,
                                ConferenceParticipant::Role newRole);
    void participantsCleared();
    void numberOfParticipantsChanged();
    void largeVideoParticipantChanged();
};
