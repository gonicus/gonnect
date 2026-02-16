pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
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
    readonly property int qualityLevel: control.callItem?.qualityLevel ?? SIPCallManager.QualityLevel.Low
    readonly property int securityLevel: control.callItem?.securityLevel ?? SIPCallManager.SecurityLevel.Low

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

    IconLabel {
        id: securityLevelIcon
        anchors {
            left: parent.left
            leftMargin: 20
            verticalCenter: parent.verticalCenter
        }
        icon {
            width: 28
            height: 28
            source: {
                switch (control.securityLevel) {
                    case SIPCallManager.SecurityLevel.Low:
                        return Icons.securityLow
                    case SIPCallManager.SecurityLevel.Medium:
                        return Icons.securityMedium
                    case SIPCallManager.SecurityLevel.High:
                        return Icons.securityHigh
                }
                throw new Error("Unknown security level value: " + control.securityLevel)
            }
        }
    }

    IconLabel {
        id: callQualityIcon
        anchors {
            left: securityLevelIcon.right
            leftMargin: 20
            verticalCenter: parent.verticalCenter
        }
        icon {
            width: 24
            height: 24
            source: {
                switch (control.qualityLevel) {
                    case SIPCallManager.QualityLevel.Low:
                        return Icons.callQualityLow
                    case SIPCallManager.QualityLevel.Medium:
                        return Icons.callQualityMed
                    case SIPCallManager.QualityLevel.High:
                        return Icons.callQualityHigh
                }
                throw new Error("Unknown quality level value: " + control.qualityLevel)
            }
        }
    }

    Rectangle {
        id: elabsedTimeSeparator
        height: 32
        width: 1
        color: Theme.borderColor
        anchors {
            left: callQualityIcon.right
            leftMargin: 20
            verticalCenter: parent.verticalCenter
        }
    }

    IconLabel {
        id: elapsedTimeLabel
        color: Theme.secondaryTextColor
        text: ViewHelper.secondsToNiceText(internal.elapsedSeconds)
        spacing: 4
        anchors {
            left: elabsedTimeSeparator.right
            leftMargin: 20
            verticalCenter: parent.verticalCenter
        }
        icon {
            color: Theme.secondaryTextColor
            source: Icons.acceptTimeEvent
            width: 20
            height: 20
        }

        Accessible.role: Accessible.StaticText
        Accessible.name: qsTr('Elapsed call time')
        Accessible.description: qsTr("The duration in seconds the call has been active for: ")
                                + ViewHelper.secondsToNiceText(internal.elapsedSeconds)
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
            onClicked: () => screenShareButton.screenShare()

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Screensharing control")
            Accessible.description: qsTr("Start or stop sharing your screen")
            Accessible.focusable: true
            Accessible.onPressAction: () => screenShareButton.screenShare()

            function screenShare() {
                ViewHelper.nextMeetingStartFlags = IConferenceConnector.StartFlag.AudioActive | IConferenceConnector.StartFlag.ScreenShareActive
                SIPCallManager.triggerCapability(control.accountId, control.callId, "jitsi:hangup")
            }
        }

        BarButton {
            id: videoMuteButton
            text: qsTr("Camera")
            iconPath: Icons.cameraOff
            enabled: control.areInCallButtonsEnabled
            visible: ViewHelper.isJitsiAvailable && control.hasCapabilityJitsi && !ViewHelper.isActiveVideoCall
            onClicked: () => videoMuteButton.videoMute()

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Camera control")
            Accessible.description: qsTr("Enabled or disable your camera")
            Accessible.focusable: true
            Accessible.onPressAction: () => videoMuteButton.videoMute()

            function videoMute() {
                ViewHelper.nextMeetingStartFlags = IConferenceConnector.StartFlag.AudioActive | IConferenceConnector.StartFlag.VideoActive
                SIPCallManager.triggerCapability(control.accountId, control.callId, "jitsi:hangup")
            }
        }

        BarButton {
            id: holdButton
            text: control.isHolding ? qsTr("Resume") : qsTr("Hold")
            iconPath: control.isHolding ? Icons.mediaPlaybackStart : Icons.mediaPlaybackPause
            enabled: control.isEstablished && !control.isFinished
            visible: control.showHoldButton && control.isEstablished
            onClicked: () => SIPCallManager.toggleHoldCall(control.accountId, control.callId)

            Accessible.role: Accessible.Button
            Accessible.name: control.isHolding ? qsTr("Resume call") : qsTr("Hold call")
            Accessible.description: qsTr("Update the call hold state")
            Accessible.focusable: true
            Accessible.onPressAction: () => SIPCallManager.toggleHoldCall(control.accountId, control.callId)
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

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Input control")
            Accessible.description: qsTr("Set the mute state of the current input device")
            Accessible.focusable: true
            Accessible.onPressAction: () => GlobalMuteState.toggleMute()
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

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Output control")
            Accessible.description: qsTr("Change the current output devices")
            Accessible.focusable: true
            Accessible.onPressAction: () => audioOutputDeviceMenu.popup(audioOutputDeviceButton, -audioOutputDeviceMenu.width + audioOutputDeviceButton.width, audioOutputDeviceButton.height)
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

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Accept call")
            Accessible.focusable: true
            Accessible.onPressAction: () => control.acceptCallClicked()
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

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Hangup call")
            Accessible.focusable: true
            Accessible.onPressAction: () => control.hangupClicked()
        }
    }
}
