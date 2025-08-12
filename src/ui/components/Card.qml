pragma ComponentBehavior: Bound

import QtQuick
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control

    Rectangle {
        id: background
        color: Theme.backgroundColor
        radius: 12
        anchors.fill: parent
    }

    DropShadow {
        anchors.fill: background
        horizontalOffset: 1
        verticalOffset: 1
        radius: 6.0
        color: Theme.shadowColor
        source: background
    }
}
