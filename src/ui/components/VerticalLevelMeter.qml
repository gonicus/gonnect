pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base


Item {
    id: control
    width: 20
    height: 20

    property bool hasAudioLevel

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

    IconLabel {
        id: iconItem
        anchors.centerIn: parent
        icon {
            source: Icons.audioVolumeHigh
            width: 12
            height: 12
        }
    }
}
