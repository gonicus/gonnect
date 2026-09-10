pragma ComponentBehavior: Bound

import QtQuick
import Qt5Compat.GraphicalEffects
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

    DropShadow {
        anchors.fill: background
        horizontalOffset: 1
        verticalOffset: 1
        radius: 6.0
        color: Theme.shadowColor
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
