pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    property alias highlighted: borderRect.visible

    Rectangle {
        id: background
        color: Theme.backgroundColor
        radius: 12
        anchors.fill: parent

        Accessible.ignored: true
    }

    CardShadow {
        anchors.fill: background
        source: background
    }

    Rectangle {
        id: borderRect
        visible: false
        radius: background.radius
        anchors.fill: background
        color: 'transparent'
        z: 50
        border {
            width: 1
            color: Theme.accentColor
        }

        Accessible.ignored: true
    }
}
