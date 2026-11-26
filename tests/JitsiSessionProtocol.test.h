#pragma once

#include <QObject>

class JitsiSessionProtocolTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesJoined_data();
    void parsesJoined();
    void parsesRoleChanged();
    void parsesUserJoined();
    void parsesUserLeft();
    void parsesDisplayNameChanged();
    void parsesParticipantMuted();
    void parsesHandRaised_data();
    void parsesHandRaised();
    void parsesDominantSpeaker();
    void parseMessageReceived_data();
    void parseMessageReceived();
    void parsesAudioMuted();
    void parsesVideoMuted();
    void parsesDevices();
    void parsesPasswordRequired();
    void parsesRoomLockChanged();
    void parsesLobbyEvents();
    void parsesScreenSharing();
    void parsesFailed();

    void rejectsInvalidEvent_data();
    void rejectsInvalidEvent();
    void namesInvalidField();

    void createsLeaveCommand();
    void createsAudioCommand();
    void createsVideoCommand();
    void createsModerationCommands();
    void createsHandCommand();
    void createsMessageCommand();
    void createsPasswordCommands();
    void createsLobbyCommands();
    void createsVideoQualityCommand();
    void createsEndConferenceCommand();
    void createsOnHoldCommand();
    void createsScreenSharingCommand();
};
