pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

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
}
