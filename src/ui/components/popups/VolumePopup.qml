pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Popup {
    id: volumePopup
    transformOrigin: Item.Top
    enter: null  // Transitions seem to cause that the popup is not visible sometimes...
    exit: null
    closePolicy: Popup.CloseOnPressOutsideParent

    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

    /// The volume of the device, between 0.0 and 1.0
    property real incomingVolume
    property bool isMuted
    property alias showMuteButton: muteButton.visible

    signal volumeChanged(real volume)
    signal muteToggled

    onIncomingVolumeChanged: () => volumeSlider.value = volumePopup.incomingVolume

    RowLayout {
        spacing: 10
        anchors {
            left: parent.left
            right: parent.right
        }

        Slider {
            id: volumeSlider
            Layout.preferredWidth: volumeSlider.implicitWidth
            Layout.preferredHeight: volumeSlider.implicitHeight

            onMoved: () => volumePopup.volumeChanged(volumeSlider.value)

            Accessible.role: Accessible.Slider
            Accessible.name: qsTr("Adjust the volume")
            Accessible.focusable: true
            Accessible.onIncreaseAction: () => {
                if (volumeSlider.value < volumeSlider.to) {
                    volumeSlider.value += volumeSlider.stepSize
                }
            }
            Accessible.onDecreaseAction: () => {
                if (volumeSlider.value > volumeSlider.from) {
                    volumeSlider.value -= volumeSlider.stepSize
                }
            }
        }

        Rectangle {
            id: muteButton
            visible: false
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            radius: 6
            color: muteButtonHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor

            Behavior on color { ColorAnimation {} }

            HoverHandler {
                id: muteButtonHandler
            }

            TapHandler {
                onTapped: () => volumePopup.muteToggled()
            }

            IconLabel {
                anchors.centerIn: parent
                icon {
                    source: volumePopup.isMuted ? Icons.microphoneSensitivityMuted : Icons.audioInputMicrophone
                    width: 16
                    height: 16
                }
            }

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Mute")
            Accessible.focusable: true
            Accessible.onPressAction: () => volumePopup.muteToggled()
        }

        Rectangle {
            id: audioSettingsButton
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            radius: 6
            color: audioSettingsButtonHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor

            Behavior on color { ColorAnimation {} }

            HoverHandler {
                id: audioSettingsButtonHandler
            }

            TapHandler {
                onTapped: () => ViewHelper.showAudioSettings()
            }

            IconLabel {
                anchors.centerIn: parent
                icon {
                    source: Icons.settingsConfigure
                    width: 16
                    height: 16
                }
            }

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Open audio settings")
            Accessible.focusable: true
            Accessible.onPressAction: () => iewHelper.showAudioSettings()
        }
    }
}
