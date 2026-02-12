pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitHeight: 64

    property bool isOnHold: false
    property bool isMuted: false
    property bool isVideoMuted: false
    property bool isSharingScreen: false
    property bool isTileView: false
    property bool isHandRaised: false
    property IConferenceConnector iConferenceConnector

    property alias videoMuteButtonVisible: videoDeviceButton.visible

    signal setOnHold(value : bool)
    signal setAudioMuted(value : bool)
    signal setVideoMuted(value : bool)
    signal setScreenShare(value : bool, screen: bool)
    signal setTileView(value : bool)
    signal setRaiseHand(value : bool)
    signal showVirtualBackgroundDialog
    signal openSetPasswordDialog
    signal openVideoQualityDialog
    signal hangup
    signal finishForAll

    QtObject {
        id: internal

        property int elapsedSeconds
        property bool hasOngoingDateEvent: false
        property int minutesRemaining

        function updateElapsedTime() {
            const jitsiConn = control.iConferenceConnector

            if (jitsiConn) {
                internal.elapsedSeconds = ViewHelper.secondsDelta(jitsiConn.establishedDateTime(), new Date())
                internal.hasOngoingDateEvent = ViewHelper.hasOngoingDateEventByRoomName(jitsiConn.roomName)

                if (internal.hasOngoingDateEvent) {
                    const endTime = ViewHelper.endTimeForOngoingDateEventByRoomName(jitsiConn.roomName)
                    internal.minutesRemaining = Math.round((endTime.getTime() - Date.now()) / 60000)
                } else {
                    internal.minutesRemaining = 0
                }
            } else {
                internal.elapsedSeconds = 0
                internal.minutesRemaining = 0
                internal.hasOngoingDateEvent = false
            }
        }

        Component.onCompleted: () => internal.updateElapsedTime()

        readonly property Timer elapsedTimeTimer: Timer {
            running: control.iConferenceConnector?.isInConference ?? false
            repeat: true
            interval: 100
            onTriggered: () => internal.updateElapsedTime()
        }
    }

    Rectangle {
        id: topBorder
        height: 1
        color: Theme.borderColor
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    Column {
        id: roomNameColumn
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 20
        }

        Label {
            anchors.left: parent.left
            text: qsTr("Conference room")
            color: Theme.secondaryTextColor
        }

        Label {
            id: roomNameLabel
            text: control.iConferenceConnector.displayName || control.iConferenceConnector.conferenceName
            anchors.left: parent.left
        }
    }

    Rectangle {
        id: timeLabelSeparator
        height: 32
        width: 1
        color: Theme.borderColor
        anchors {
            left: roomNameColumn.right
            leftMargin: 20
            verticalCenter: parent.verticalCenter
        }
    }

    Column {
        id: timeLabelContainer
        width: remainingTimeLabel.visible
               ? Math.max(elapsedTimeLabel.implicitWidth, remainingTimeLabel.implicitWidth)
               : elapsedTimeLabel.implicitWidth
        anchors {
            verticalCenter: parent.verticalCenter
            left: timeLabelSeparator.right
            leftMargin: 20
        }

        IconLabel {
            id: elapsedTimeLabel
            color: Theme.secondaryTextColor
            text: ViewHelper.secondsToNiceText(internal.elapsedSeconds)
            spacing: 4
            icon {
                color: Theme.secondaryTextColor
                source: Icons.acceptTimeEvent
                width: 20
                height: 20
            }
        }

        Label {
            id: remainingTimeLabel
            visible: internal.hasOngoingDateEvent
            text: "(" + qsTr("%n minutes left", "", internal.minutesRemaining) + ")"
            color: Theme.secondaryTextColor
        }
    }

    Flickable {
        id: rowFlickable
        width: Math.min(buttonRow.implicitWidth, control.width - (timeLabelContainer.x + timeLabelContainer.width) - rightStickyButtonRow.implicitWidth)
        contentWidth: buttonRow.implicitWidth
        clip: true
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: rightStickyButtonRow.left
        }

        ScrollBar.horizontal: ScrollBar {
            height: 5
            policy: rowFlickable.contentWidth > rowFlickable.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }

        Row {
            id: buttonRow
            spacing: 5
            rightPadding: 20
            anchors {
                top: parent.top
                bottom: parent.bottom
            }

            BarButton {
                id: shareButton
                visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Sharable)
                enabled: !control.isOnHold
                text: qsTr("Share")
                iconPath: Icons.documentShare
                showDropdownButton: true
                onClicked: () => shareButton.openShareMenu()
                onDropDownClicked: () => shareButton.openShareMenu()

                function openShareMenu() {
                    shareMenu.popup(shareButton, -shareMenu.width + shareButton.width, shareButton.height)
                }

                Menu {
                    id: shareMenu

                    MenuItem {
                        text: qsTr("Copy room name")
                        onTriggered: () => ViewHelper.copyToClipboard(control.iConferenceConnector.conferenceName)
                    }
                    MenuItem {
                        text: qsTr("Copy room link")
                        onTriggered: () => ViewHelper.copyToClipboard(control.iConferenceConnector.conferenceUrl)
                    }
                    MenuItem {
                        text: qsTr("Open in browser")
                        onTriggered: () => Qt.openUrlExternally(control.iConferenceConnector.conferenceUrl)
                    }

                    // MenuItem {
                    //     text: qsTr("Send link via email")
                    //     onTriggered: () => console.log(category, 'TODO: Send invitation link via mail')
                    // }
                }
            }

            BarButton {
                id: raiseHandButton
                visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.RaiseHand)
                enabled: !control.isOnHold
                text: qsTr("Raise")
                iconPath: Icons.transformBrowse
                iconText: control.isHandRaised ? "!" : ""
                onClicked: () => control.setRaiseHand(!control.isHandRaised)
            }

            BarButton {
                id: holdButton
                visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Holdable)
                text: control.isOnHold ? qsTr("Resume") : qsTr("Hold")
                iconPath: control.isOnHold ? Icons.mediaPlaybackStart : Icons.mediaPlaybackPause
                onClicked: () => control.setOnHold(!control.isOnHold)
            }

            BarButton {
                id: tileViewButton
                visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.TileView)
                enabled: !control.isOnHold
                text: qsTr("View")
                iconPath: control.isTileView ? Icons.viewLeftNew : Icons.viewGrid
                onClicked: () => control.setTileView(!control.isTileView)
            }

            BarButton {
                id: screenShareButton
                visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.ScreenShare)
                enabled: !control.isOnHold
                text: qsTr("Screen")
                iconPath: control.isSharingScreen ? Icons.mediaPlaybackStopped : Icons.inputTouchscreen
                onClicked: () => {
                    if (control.isSharingScreen) {
                        control.setScreenShare(false, false)
                    } else {
                        control.setScreenShare(true, true)

                        //TODO: this can be re-enabled after QTBUG-142040 / QTBUG-142040 are fixed
                        //      see Conference.qml +293
                        //screenShareMenu.popup(screenShareButton, -screenShareButton.width + screenShareButton.width, screenShareButton.height)
                    }
                }

                Menu {
                    id: screenShareMenu
                    MenuItem {
                        icon.source: Icons.window
                        text: qsTr("Share window")
                        onTriggered: () => control.setScreenShare(true, false)
                    }
                    MenuItem {
                        icon.source: Icons.screen
                        text: qsTr("Share screen")
                        onTriggered: () => control.setScreenShare(true, true)
                    }
                }
            }

            BarButton {
                id: videoDeviceButton
                visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.VideoMute)
                enabled: !control.isOnHold
                text: qsTr("Camera")
                iconPath: control.isVideoMuted ? Icons.cameraOff : Icons.cameraOn
                showDropdownButton: true
                onClicked: () => control.setVideoMuted(!control.isVideoMuted)
                onDropDownClicked: () => videoDeviceMenu.popup(videoDeviceButton, -videoDeviceMenu.width + videoDeviceButton.width, videoDeviceButton.height)

                VideoDeviceMenu {
                    id: videoDeviceMenu
                    selectedDeviceId: VideoManager.selectedDeviceId
                    onDeviceSelected: deviceId => VideoManager.selectedDeviceId = deviceId
                    onVirtualBackgroundButtonClicked: () => control.showVirtualBackgroundDialog()
                }
            }

            BarButton {
                id: audioInputDeviceButton
                visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.AudioMute)
                enabled: !control.isOnHold
                text: qsTr("Micro")
                iconPath: control.isMuted ? Icons.microphoneSensitivityMuted : Icons.audioInputMicrophone
                showDropdownButton: true
                onClicked: () => control.setAudioMuted(!control.isMuted)
                onDropDownClicked: () => audioInputDeviceMenu.popup(audioInputDeviceButton, -audioInputDeviceMenu.width + audioInputDeviceButton.width, audioInputDeviceButton.height)


                AudioDeviceMenu {
                    id: audioInputDeviceMenu
                    inputDevices: true
                    selectedDeviceId: AudioManager.captureDeviceId

                    onDeviceSelected: deviceId => AudioManager.captureDeviceId = deviceId
                }
            }

            BarButton {
                id: audioOutputDeviceButton
                enabled: !control.isOnHold
                text: qsTr("Output")
                iconPath: Icons.audioVolumeHigh
                showDropdownButton: true
                onClicked: () => audioOutputDeviceMenu.popup(audioOutputDeviceButton, -audioOutputDeviceMenu.width + audioOutputDeviceButton.width, audioOutputDeviceButton.height)
                onDropDownClicked: () => audioOutputDeviceMenu.popup(audioOutputDeviceButton, -audioOutputDeviceMenu.width + audioOutputDeviceButton.width, audioOutputDeviceButton.height)

                AudioDeviceMenu {
                    id: audioOutputDeviceMenu
                    inputDevices: false
                    selectedDeviceId: AudioManager.playbackDeviceId

                    onDeviceSelected: deviceId => AudioManager.playbackDeviceId = deviceId
                }
            }

            BarButton {
                id: moreButton
                enabled: !control.isOnHold
                text: qsTr("More")
                iconPath: Icons.applicationMenu
                showDropdownButton: true
                onClicked: () => moreMenu.popup(moreButton, -moreMenu.width + moreButton.width, moreButton.height)
                onDropDownClicked: () => moreMenu.popup(moreButton, -moreMenu.width + moreButton.width, moreButton.height)

                Menu {
                    id: moreMenu

                    MenuItem {
                        id: noiseSuppressionMenuItem
                        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.NoiseSuppression)
                        text: qsTr("Noise supression")
                        icon.source: control.iConferenceConnector.isNoiseSuppressionEnabled ? Icons.checkbox : ""
                        onClicked: () => control.iConferenceConnector.setNoiseSuppressionEnabled(!control.iConferenceConnector.isNoiseSuppressionEnabled)
                    }

                    MenuItem {
                        text: qsTr("Toggle subtitles")
                        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Subtitles)
                        icon.source: control.iConferenceConnector.isSubtitlesEnabled ? Icons.checkbox : ""
                        onClicked: () => control.iConferenceConnector.setSubtitlesEnabled(!control.iConferenceConnector.isSubtitlesEnabled)
                    }

                    MenuItem {
                        text: qsTr("Toggle whiteboard")
                        visible: control.iConferenceConnector.hasWhiteboard
                        onClicked: () => control.iConferenceConnector.toggleWhiteboard()
                    }

                    MenuItem {
                        text: qsTr("Video quality...")
                        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.VideoQualityAdjustable)
                        onClicked: () => control.openVideoQualityDialog()
                    }

                    MenuItem {
                        id: setPasswordMenuItem
                        visible: control.iConferenceConnector.ownRole === ConferenceParticipant.Role.Moderator
                                 && control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.RoomPassword)
                        text: qsTr("Set room password...")
                        onClicked: () => control.openSetPasswordDialog()
                    }

                    MenuItem {
                        visible: control.iConferenceConnector.ownRole === ConferenceParticipant.Role.Moderator
                                 && control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.MuteAll)
                        text: qsTr("Mute everyone")
                        onClicked: () => control.iConferenceConnector.muteAll()
                    }
                }
            }
        }
    }

    Row {
        id: rightStickyButtonRow
        spacing: 5
        rightPadding: 20
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }

        Rectangle {
            height: 32
            width: 1
            color: Theme.borderColor
            anchors.verticalCenter: parent.verticalCenter
        }

        Button {
            id: hangupButton
            width: 50
            height: 50
            highlighted: true
            anchors.verticalCenter: parent.verticalCenter
            icon.source: Icons.callStop

            Material.accent: Theme.redColor

            Component.onCompleted: () => {
                hangupButton.icon.width = 24
                hangupButton.icon.height = 24
            }

            onClicked: () => {
                const conn = control.iConferenceConnector

                if (conn.ownRole === ConferenceParticipant.Role.Moderator && conn.numberOfParticipants > 1) {
                    leaveMenu.popup(hangupButton, -leaveMenu.width + hangupButton.width, hangupButton.height)
                } else {
                    hangupButton.enabled = false
                    control.hangup()
                }
            }

            Menu {
                id: leaveMenu

                MenuItem {
                    text: qsTr("Leave conference")
                    onClicked: () => {
                        hangupButton.enabled = false
                        control.hangup()
                    }
                }

                MenuItem {
                    text: qsTr("End conference for all")
                    onClicked: () => {
                        hangupButton.enabled = false
                        control.finishForAll()
                    }
                }
            }
        }
    }
}
