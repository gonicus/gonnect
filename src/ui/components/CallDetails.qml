import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    focus: true

    property CallItem callItem

    readonly property int callId: control.callItem?.callId ?? -1
    readonly property string contactName: control.callItem?.contactName ?? ""
    readonly property string accountId: control.callItem?.accountId ?? ""
    readonly property bool isEstablished: control.callItem?.isEstablished ?? false
    readonly property date establishedTime: control.callItem?.establishedTime ?? new Date()
    readonly property bool isHolding: control.callItem?.isHolding ?? false
    readonly property bool isFinished: control.callItem?.isFinished ?? false
    readonly property bool hasCapabilityJitsi: control.callItem?.hasCapabilityJitsi ?? false
    readonly property int statusCode: control.callItem?.statusCode ?? 0
    readonly property bool hasIncomingAudioLevel: control.callItem?.hasIncomingAudioLevel ?? false
    readonly property bool showHoldButton: control.callItem?.showHoldButton ?? true

    Keys.onPressed: (event) => {
                        if (event.isAutoRepeat || !control.callItem || !control.isEstablished || control.isFinished) {
                            return
                        }

                        const key = event.text.toUpperCase()
                        const dtmfKeys = [ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "#", "*", "A", "B", "C", "D" ]

                        if (dtmfKeys.includes(key)) {
                            event.accepted = true
                            SIPCallManager.sendDtmf(control.accountId, control.callId, key)

                            dtmfFeedbackLabel.text = key
                            internal.dtmfAnimation.restart()
                        }
                    }

    Component.onCompleted: () => control.forceActiveFocus()

    QtObject {
        id: internal

        property int elapsedSeconds

        readonly property list<int> niceStatusCodes: [ 180, 403, 486 ]

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

        readonly property SequentialAnimation dtmfAnimation: SequentialAnimation {
            NumberAnimation {
                target: dtmfFeedbackRect
                property: 'opacity'
                to: 1.0
                duration: 100
            }
            NumberAnimation {
                target: dtmfFeedbackRect
                property: 'opacity'
                easing.type: Easing.InQuad
                to: 0.0
                duration: 1200
            }
        }
    }

    AvatarImage {
        id: avatarImage
        size: 120
        initials: ViewHelper.initials(control.contactName)
        source: control.callItem?.hasAvatar ? ("file://" + control.callItem.avatarPath) : ""
        anchors {
            centerIn: parent
            verticalCenterOffset: -25
        }
    }

    Label {
        id: elapsedTimeLabel
        font.pixelSize: 22
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: avatarImage.bottom
            topMargin: 25
        }
        text: internal.niceStatusCodes.includes(control.statusCode)
              ? EnumTranslation.sipStatusCode(control.statusCode)
              : ("🕓  " + ViewHelper.secondsToNiceText(internal.elapsedSeconds))
    }

    Label {
        id: statusCodeLabel
        visible: control.statusCode < 200 && control.statusCode >= 300 && !internal.niceStatusCodes.includes(control.statusCode)
        text: `${EnumTranslation.sipStatusCode(control.statusCode)} (${control.statusCode})`
        color: Theme.secondaryTextColor
        anchors {
            top: elapsedTimeLabel.bottom
            topMargin: 15
            horizontalCenter: elapsedTimeLabel.horizontalCenter
        }

        Accessible.role: Accessible.StaticText
        Accessible.name: qsTr("SIP call status code")
        Accessible.description: qsTr("The current status code of the call: ")
                                + statusCodeLabel.text
    }

    AudioLevelButton {
        id: micLevel
        showMuteButton: true
        isMuted: AudioManager.isAudioCaptureMuted
        iconSource: micLevel.isMuted ? Icons.microphoneSensitivityMuted : Icons.audioInputMicrophone
        incomingVolume: AudioManager.captureAudioVolume
        hasAudioLevel: AudioManager.hasCaptureAudioLevel
        anchors {
            top: parent.top
            topMargin: 15
            right: parent.right
            rightMargin: 15
        }

        onVolumeChanged: (volume) => AudioManager.captureAudioVolume = volume
        onMuteToggled: () => AudioManager.isAudioCaptureMuted = !AudioManager.isAudioCaptureMuted
    }

    AudioLevelButton {
        id: outputLevel
        iconSource: Icons.audioVolumeHigh
        incomingVolume: AudioManager.playbackAudioVolume
        hasAudioLevel: control.hasIncomingAudioLevel
        anchors {
            top: micLevel.top
            right: micLevel.left
            rightMargin: 15
        }

        onVolumeChanged: (volume) => AudioManager.playbackAudioVolume = volume
    }

    Rectangle {
        id: dtmfFeedbackRect
        anchors.centerIn: parent
        width: 100
        height: 100
        radius: 12
        color: Theme.backgroundColor
        opacity: 0

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Theme.backgroundOffsetColor
        }

        Label {
            id: dtmfFeedbackLabel
            anchors.centerIn: parent
            font.pixelSize: 50
        }
    }

    Button {
        id: jitsiButton
        text: qsTr('Jitsi Meet')
        visible: control.hasCapabilityJitsi
        icon {
            source: Icons.jitsi
            color: "transparent"
            height: 22
        }
        anchors {
            verticalCenter: holdButton.verticalCenter
            left: parent.left
            leftMargin: 10
        }
        onClicked: () => SIPCallManager.triggerCapability(control.accountId, control.callId, "jitsi")

        Accessible.role: Accessible.Button
        Accessible.name: jitsiButton.text
        Accessible.description: qsTr("Switch to a Jitis Meet session")
        Accessible.focusable: true
        Accessible.onPressAction: () => jitsiButton.clicked()
    }

    Button {
        id: holdButton
        text: control.isHolding ? qsTr("Unhold") : qsTr("Hold")
        highlighted: control.isHolding
        enabled: control.isEstablished && !control.isFinished
        visible: control.showHoldButton && control.isEstablished
        anchors {
            verticalCenter: hangupButton.verticalCenter
            right: hangupButton.left
            rightMargin: 10
        }
        onClicked: () => {
            if (control.isHolding) {
                SIPCallManager.unholdCall(control.accountId, control.callId)
            } else {
                SIPCallManager.holdCall(control.accountId, control.callId)
            }
        }

        Accessible.role: Accessible.Button
        Accessible.name: holdButton.text
        Accessible.description: qsTr("Toggle the hold state to: ") + holdButton.text
        Accessible.focusable: true
        Accessible.onPressAction: () => holdButton.clicked()
    }

    Button {
        id: acceptButton
        Material.accent: Theme.greenColor
        highlighted: true
        icon.source: Icons.callStart
        enabled: !!control.callItem && !control.isFinished
        visible: !!control.callItem && control.callItem.isIncoming && !control.isEstablished

        anchors {
            verticalCenter: hangupButton.verticalCenter
            right: hangupButton.left
            rightMargin: 10
        }

        onClicked: () => SIPCallManager.acceptCall(control.accountId, control.callId)

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Accept call")
        Accessible.focusable: true
        Accessible.onPressAction: () => acceptButton.clicked()
    }

    Button {
        id: hangupButton
        Material.accent: Theme.redColor
        highlighted: true
        icon.source: Icons.callStop
        enabled: !!control.callItem && !control.isFinished
        anchors {
            bottom: parent.bottom
            bottomMargin: 5
            right: parent.right
            rightMargin: 10
        }
        onClicked: () => {
            if (SIPCallManager.isConferenceMode) {
                SIPCallManager.endConference()
            } else {
                SIPCallManager.endCall(control.accountId, control.callId)
            }
        }

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Hangup call")
        Accessible.focusable: true
        Accessible.onPressAction: () => hangupButton.clicked()
    }
}
