pragma ComponentBehavior: Bound

import QtQuick
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
    property JitsiConnector jitsiConnector

    property alias videoMuteButtonVisible: videoDeviceButton.visible

    signal toggleHold
    signal toggleMute
    signal toggleVideoMute
    signal toggleVirtualBackgroundDialog
    signal toggleScreenShare
    signal toggleTileView
    signal toggleRaiseHand
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
            const jitsiConn = control.jitsiConnector

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
            running: control.jitsiConnector?.isInRoom ?? false
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
        id: timeLabelContainer
        width: Math.max(elapsedTimeLabel.implicitWidth, remainingTimeLabel.implicitWidth)
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 20
        }

        Label {
            id: elapsedTimeLabel
            color: Theme.secondaryTextColor
            text: "🕓 " + ViewHelper.secondsToNiceText(internal.elapsedSeconds)
        }

        Label {
            id: remainingTimeLabel
            visible: internal.hasOngoingDateEvent
            text: "(" + qsTr("%n minutes left", "", internal.minutesRemaining) + ")"
            color: Theme.secondaryTextColor
        }
    }

    Column {
        id: roomNameColumn
        anchors {
            verticalCenter: parent.verticalCenter
            left: timeLabelContainer.right
            leftMargin: 20
        }

        Label {
            anchors.left: parent.left
            text: qsTr("Conference room")
            color: Theme.secondaryTextColor
        }

        Label {
            id: roomNameLabel
            text: control.jitsiConnector.displayName || control.jitsiConnector.roomName
            anchors.left: parent.left
        }
    }

    Flickable {
        id: rowFlickable
        width: Math.min(buttonRow.implicitWidth, control.width - (roomNameLabel.x + roomNameLabel.width))
        contentWidth: buttonRow.implicitWidth
        clip: true
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
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
                        onTriggered: () => ViewHelper.copyToClipboard(control.jitsiConnector.roomName)
                    }
                    MenuItem {
                        text: qsTr("Copy room link")
                        onTriggered: () => ViewHelper.copyToClipboard(control.jitsiConnector.roomUrl)
                    }
                    MenuItem {
                        text: qsTr("Open in browser")
                        onTriggered: () => Qt.openUrlExternally(control.jitsiConnector.roomUrl)
                    }

                    // MenuItem {
                    //     text: qsTr("Send link via email")
                    //     onTriggered: () => console.log('TODO: Send invitation link via mail')
                    // }
                }
            }

            BarButton {
                id: raiseHandButton
                enabled: !control.isOnHold
                text: qsTr("Raise")
                iconPath: Icons.transformBrowse
                iconText: control.isHandRaised ? "!" : ""
                onClicked: () => control.toggleRaiseHand()
            }

            BarButton {
                id: holdButton
                text: control.isOnHold ? qsTr("Resume") : qsTr("Hold")
                iconPath: control.isOnHold ? Icons.mediaPlaybackStart : Icons.mediaPlaybackPause
                onClicked: () => control.toggleHold()
            }

            BarButton {
                id: tileViewButton
                enabled: !control.isOnHold
                text: qsTr("View")
                iconPath: control.isTileView ? Icons.viewLeftNew : Icons.viewGrid
                onClicked: () => control.toggleTileView()
            }

            BarButton {
                id: screenShareButton
                enabled: !control.isOnHold
                text: qsTr("Screen")
                iconPath: control.isSharingScreen ? Icons.mediaPlaybackStopped : Icons.inputTouchscreen
                onClicked: () => control.toggleScreenShare()
            }

            BarButton {
                id: videoDeviceButton
                enabled: !control.isOnHold
                text: qsTr("Camera")
                iconPath: control.isVideoMuted ? Icons.cameraOff : Icons.cameraOn
                showDropdownButton: true
                onClicked: () => control.toggleVideoMute()
                onDropDownClicked: () => videoDeviceMenu.popup(videoDeviceButton, -videoDeviceMenu.width + videoDeviceButton.width, videoDeviceButton.height)

                VideoDeviceMenu {
                    id: videoDeviceMenu
                    selectedDeviceId: VideoManager.selectedDeviceId
                    onDeviceSelected: deviceId => VideoManager.selectedDeviceId = deviceId
                    onVirtualBackgroundButtonClicked: () => control.toggleVirtualBackgroundDialog()
                }
            }

            BarButton {
                id: audioInputDeviceButton
                enabled: !control.isOnHold
                text: qsTr("Micro")
                iconPath: control.isMuted ? Icons.microphoneSensitivityMuted : Icons.audioInputMicrophone
                showDropdownButton: true
                onClicked: () => control.toggleMute()
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
                        text: qsTr("Noise supression")
                        icon.source: control.jitsiConnector.isNoiseSupression ? Icons.checkbox : ""
                        onClicked: () => control.jitsiConnector.toggleNoiseSupression()
                    }

                    MenuItem {
                        text: qsTr("Video quality...")
                        onClicked: () => control.openVideoQualityDialog()
                    }

                    MenuItem {
                        id: setPasswordMenuItem
                        visible: control.jitsiConnector.isModerator
                        text: qsTr("Set room password...")
                        onClicked: () => control.openSetPasswordDialog()
                    }

                    MenuItem {
                        visible: control.jitsiConnector.isModerator
                        text: qsTr("Mute everyone")
                        onClicked: () => control.jitsiConnector.muteAll()
                    }
                }
            }

            Rectangle {
                visible: raiseHandButton.visible
                         || tileViewButton.visible
                         || audioInputDeviceButton.visible
                         || audioOutputDeviceButton.visible
                         || videoDeviceButton.visible
                         || screenShareButton.visible
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
                    if (control.jitsiConnector.isModerator && control.jitsiConnector.numberOfParticipants > 1) {
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
}
