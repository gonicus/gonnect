pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Menu {
    id: control

    property IConferenceConnector iConferenceConnector

    signal openSetPasswordDialog
    signal openVideoQualityDialog
    signal showVirtualBackgroundDialog

    readonly property bool isOnHold: control.iConferenceConnector?.isOnHold ?? false
    readonly property bool isVideoAvailable: control.iConferenceConnector?.isVideoAvailable ?? false

    HideableMenuItem {
        id: audioOutputDeviceButton
        enabled: !control.isOnHold
        text: qsTr("Output...")
        icon.source: Icons.audioVolumeHigh
        onClicked: () => audioOutputDeviceMenu.popup(control.parent, -audioOutputDeviceMenu.width + audioOutputDeviceButton.width, audioOutputDeviceButton.height)

        AudioDeviceMenu {
            id: audioOutputDeviceMenu
            inputDevices: false
            selectedDeviceId: AudioManager.playbackDeviceId

            onDeviceSelected: deviceId => AudioManager.playbackDeviceId = deviceId
        }
    }

    HideableMenuItem {
        id: audioInputDeviceButton
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.AudioMute)
        enabled: !control.isOnHold
        text: qsTr("Microphone...")
        icon.source: Icons.audioInputMicrophone
        onClicked: () => audioInputDeviceMenu.popup(control.parent, -audioInputDeviceMenu.width + audioInputDeviceButton.width, audioInputDeviceButton.height)

        AudioDeviceMenu {
            id: audioInputDeviceMenu
            inputDevices: true
            selectedDeviceId: AudioManager.captureDeviceId

            onDeviceSelected: deviceId => AudioManager.captureDeviceId = deviceId
        }
    }

    HideableMenuItem {
        id: videoDeviceButton
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.VideoMute) && control.isVideoAvailable
        enabled: !control.isOnHold
        text: qsTr("Camera...")
        icon.source: Icons.cameraOn
        onClicked: () => videoDeviceMenu.popup(control.parent, -videoDeviceMenu.width + videoDeviceButton.width, videoDeviceButton.height)

        VideoDeviceMenu {
            id: videoDeviceMenu
            selectedDeviceId: VideoManager.selectedDeviceId
            onDeviceSelected: deviceId => VideoManager.selectedDeviceId = deviceId
            onVirtualBackgroundButtonClicked: () => control.showVirtualBackgroundDialog()
        }
    }

    MenuSeparator {}

    HideableMenuItem {
        id: noiseSuppressionMenuItem
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.NoiseSuppression)
        text: qsTr("Noise supression")
        icon.source: control.iConferenceConnector.isNoiseSuppressionEnabled ? Icons.checkbox : Icons.noisereduction
        onClicked: () => control.iConferenceConnector.setNoiseSuppressionEnabled(!control.iConferenceConnector.isNoiseSuppressionEnabled)
    }

    HideableMenuItem {
        text: qsTr("Video quality...")
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.VideoQualityAdjustable)
        icon.source: Icons.settingsConfigure
        onClicked: () => control.openVideoQualityDialog()
    }

    MenuSeparator {}

    HideableMenuItem {
        text: qsTr("Toggle subtitles")
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Subtitles)
        icon.source: control.iConferenceConnector.isSubtitlesEnabled ? Icons.checkbox : Icons.addSubtitle
        onClicked: () => control.iConferenceConnector.setSubtitlesEnabled(!control.iConferenceConnector.isSubtitlesEnabled)
    }

    HideableMenuItem {
        text: qsTr("Toggle whiteboard")
        visible: control.iConferenceConnector.hasWhiteboard
        icon.source: Icons.drawFreehand
        onClicked: () => control.iConferenceConnector.toggleWhiteboard()
    }

    HideableMenuItem {
        id: setPasswordMenuItem
        visible: control.iConferenceConnector.ownRole === ConferenceUser.Role.Moderator
                 && control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.RoomPassword)
        text: qsTr("Set room password...")
        icon.source: Icons.documentEncrypted
        onClicked: () => control.openSetPasswordDialog()
    }

    HideableMenuItem {
        visible: control.iConferenceConnector.ownRole === ConferenceUser.Role.Moderator
                 && control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.MuteAll)
        text: qsTr("Mute everyone")
        icon.source: Icons.microphoneSensitivityMuted
        onClicked: () => control.iConferenceConnector.muteAll()
    }

    MenuSeparator {
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Sharable)
    }

    HideableMenuItem {
        text: qsTr("Copy room name")
        icon.source: Icons.editCopy
        enabled: !control.isOnHold
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Sharable)
        onTriggered: () => ClipboardHelper.copyToClipboard(control.iConferenceConnector.conferenceName)
    }
    HideableMenuItem {
        text: qsTr("Copy room link")
        icon.source: Icons.editCopy
        enabled: !control.isOnHold
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Sharable)
        onTriggered: () => ClipboardHelper.copyToClipboard(control.iConferenceConnector.conferenceUrl)
    }
    HideableMenuItem {
        text: qsTr("Open in browser")
        icon.source: Icons.openLink
        enabled: !control.isOnHold
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Sharable)
        onTriggered: () => Qt.openUrlExternally(control.iConferenceConnector.conferenceUrl)
    }
    HideableMenuItem {
        text: qsTr("Show phone number")
        icon.source: Icons.callStart
        enabled: (control.iConferenceConnector?.hasDialIn ?? false) && !control.isOnHold
        visible: control.iConferenceConnector.hasCapability(IConferenceConnector.Capability.Sharable)
        onTriggered: () => control.iConferenceConnector.requestDialInInfo()
    }

    // MenuItem {
    //     text: qsTr("Send link via email")
    //     onTriggered: () => console.log(category, 'TODO: Send invitation link via mail')
    // }

}
