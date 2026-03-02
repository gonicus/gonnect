pragma ComponentBehavior: Bound

import QtQuick
import base

Rectangle {
    id: control
    radius: 8
    implicitHeight: 30
    color: Theme.paneColor

    readonly property alias count: togglerList.count

    Accessible.role: Accessible.StatusBar
    Accessible.name: qsTr("Status bar")

    Rectangle {
        height: control.radius
        color: Theme.paneColor
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Accessible.ignored: true
    }

    Rectangle {
        height: 1
        color: Theme.borderColor
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Accessible.ignored: true
    }

    TogglerList {
        id: togglerList
        anchors.fill: parent
        clip: true
    }
}
