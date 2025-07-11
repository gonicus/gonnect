pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base


Item {
    id: control
    implicitWidth: 32
    implicitHeight: 32

    property alias showMuteButton: volumePopup.showMuteButton
    property bool isMuted: false

    /// If an audio level above a certain threshold is detected
    property bool hasAudioLevel

    /// The volume of the device, between 0.0 and 1.0
    property alias incomingVolume: volumePopup.incomingVolume
    property alias iconSource: iconItem.icon.source

    signal volumeChanged(real volume)
    signal muteToggled


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

    VolumePopup {
        id: volumePopup
        x: -(volumePopup.width - parent.width) + 8
        y: control.height + 12
        isMuted: control.isMuted

        Material.accent: control.Material.accent
        Material.primary: control.Material.primary

        onVolumeChanged: volume => control.volumeChanged(volume)
        onMuteToggled: () => control.muteToggled()
    }
}
