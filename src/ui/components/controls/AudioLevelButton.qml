pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base


Item {
    id: control
    implicitWidth: 32
    implicitHeight: 32

    property alias showMuteButton: muteButton.visible
    property bool isMuted: false

    /// If an audio level above a certain threshold is detected
    property bool hasAudioLevel

    /// The volume of the device, between 0.0 and 1.0
    property real incomingVolume
    property alias iconSource: iconItem.icon.source

    signal volumeChanged(real volume)
    signal muteToggled

    onIncomingVolumeChanged: () => volumeSlider.value = control.incomingVolume

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: bg.width / 2
        color: Theme.backgroundOffsetHoveredColor
        opacity: control.hasAudioLevel ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }
    }

    Rectangle {
        id: hoverBackground
        visible: hoverHandler.hovered
        radius: bg.radius
        color: Theme.backgroundOffsetHoveredColor
        anchors.fill: bg
    }

    IconLabel {
        id: iconItem
        anchors.centerIn: parent
        icon {
            width: 16
            height: 16
        }
    }

    HoverHandler {
        id: hoverHandler
    }

    TapHandler {
        gesturePolicy: TapHandler.WithinBounds
        grabPermissions: PointerHandler.ApprovesTakeOverByAnything
        exclusiveSignals: TapHandler.SingleTap
        onSingleTapped: () => {
                            if (volumePopup.visible) {
                                volumePopup.close()
                            } else {
                                volumePopup.open()
                            }
                        }
    }

    Popup {
        id: volumePopup
        x: -(volumePopup.width - parent.width) + 8
        y: control.height + 12
        transformOrigin: Item.Top
        enter: null  // Transitions seem to cause that the popup is not visible sometimes...
        exit: null
        closePolicy: Popup.CloseOnPressOutsideParent

        Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light
        Material.accent: control.Material.accent
        Material.primary: control.Material.primary

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

                onMoved: () => control.volumeChanged(volumeSlider.value)
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
                    onTapped: () => control.muteToggled()
                }

                IconLabel {
                    anchors.centerIn: parent
                    icon {
                        source: control.isMuted ? Icons.microphoneSensitivityMuted : Icons.audioInputMicrophone
                        width: 16
                        height: 16
                    }
                }
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
            }
        }
    }
}
