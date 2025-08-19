pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitHeight: 64

    signal hangupClicked
    signal acceptCallClicked

    property CallItem callItem
    property alias jitsiButtonVisible: videoMuteButton.visible

    readonly property int callId: control.callItem?.callId ?? -1
    readonly property string accountId: control.callItem?.accountId ?? ""
    readonly property bool showHoldButton: control.callItem?.showHoldButton ?? true
    readonly property bool isEstablished: control.callItem?.isEstablished ?? false
    readonly property bool isHolding: control.callItem?.isHolding ?? false
    readonly property bool isFinished: control.callItem?.isFinished ?? false
    readonly property bool isIncoming: control.callItem?.isIncoming ?? false
    readonly property bool hasCapabilityJitsi: control.callItem?.hasCapabilityJitsi ?? false

    readonly property bool areInCallButtonsEnabled: control.isEstablished && !control.isFinished

    QtObject {
        id: internal

        property int elapsedSeconds

        function updateElapsedTime() {
            const callItem = control.callItem
            if (callItem) {
                internal.elapsedSeconds = ViewHelper.secondsDelta(callItem.establishedTime, new Date())
            } else {
                internal.elapsedSeconds = 0
            }
        }

        Component.onCompleted: () => internal.updateElapsedTime()

        readonly property Timer elapsedTimeTimer: Timer {
            running: !!control.callItem && !control.isFinished
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

    Label {
        id: elapsedTimeLabel
        color: Theme.secondaryTextColor
        text: "🕓  " + ViewHelper.secondsToNiceText(internal.elapsedSeconds)
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 20
        }
    }

    Row {
        spacing: 5
        rightPadding: 20
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }

        BarButton {
            id: screenShareButton
            text: qsTr("Screen")
            iconPath: Icons.inputTouchscreen
            enabled: control.areInCallButtonsEnabled
            visible: ViewHelper.isJitsiAvailable && control.hasCapabilityJitsi && !ViewHelper.isActiveVideoCall
            onClicked: () => {
                ViewHelper.nextMeetingStartFlags = JitsiConnector.MeetingStartFlag.AudioActive | JitsiConnector.MeetingStartFlag.ScreenShareActive
                SIPCallManager.triggerCapability(control.accountId, control.callId, "jitsi:hangup")
            }
        }

        BarButton {
            id: videoMuteButton
            text: qsTr("Camera")
            iconPath: Icons.cameraOff
            enabled: control.areInCallButtonsEnabled
            visible: ViewHelper.isJitsiAvailable && control.hasCapabilityJitsi && !ViewHelper.isActiveVideoCall
            onClicked: () => {
                ViewHelper.nextMeetingStartFlags = JitsiConnector.MeetingStartFlag.AudioActive | JitsiConnector.MeetingStartFlag.VideoActive
                SIPCallManager.triggerCapability(control.accountId, control.callId, "jitsi:hangup")
            }
        }

        BarButton {
            id: holdButton
            text: control.isHolding ? qsTr("Resume") : qsTr("Hold")
            iconPath: control.isHolding ? Icons.mediaPlaybackStart : Icons.mediaPlaybackPause
            enabled: control.isEstablished && !control.isFinished
            visible: control.showHoldButton && control.isEstablished
            onClicked: () => GlobalCallState.triggerHold()
        }

        Rectangle {
            visible: holdButton.visible || videoMuteButton.visible
            height: 32
            width: 1
            color: Theme.borderColor
            enabled: control.areInCallButtonsEnabled
            anchors.verticalCenter: parent.verticalCenter
        }

        BarButton {
            id: audioInputDeviceButton
            text: qsTr("Micro")
            iconPath: AudioManager.isAudioCaptureMuted ? Icons.microphoneSensitivityMuted : Icons.audioInputMicrophone
            enabled: control.areInCallButtonsEnabled
            showDropdownButton: true
            onClicked: () => GlobalMuteState.toggleMute()
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
            text: qsTr("Output")
            iconPath: Icons.audioVolumeHigh
            enabled: control.areInCallButtonsEnabled
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

        Button {
            id: acceptCallButton
            width: 50
            height: 50
            highlighted: true
            anchors.verticalCenter: parent.verticalCenter
            icon.source: Icons.callStart
            visible: !control.isEstablished && !control.isFinished && control.isIncoming

            Material.accent: Theme.greenColor

            Component.onCompleted: () => {
                acceptCallButton.icon.width = 24
                acceptCallButton.icon.height = 24
            }

            onClicked: () => control.acceptCallClicked()
        }

        Button {
            id: hangupButton
            width: 50
            height: 50
            highlighted: true
            anchors.verticalCenter: parent.verticalCenter
            icon.source: Icons.callStop
            enabled: SIPCallManager.isConferenceMode || !control.isFinished

            Material.accent: Theme.redColor

            Component.onCompleted: () => {
                hangupButton.icon.width = 24
                hangupButton.icon.height = 24
            }

            onClicked: () => control.hangupClicked()
        }
    }
}
