pragma ComponentBehavior: Bound

import QtQuick
import base

Menu {
    id: control

    HideableMenuItem {
        id: audioOutputDeviceButton
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
}
