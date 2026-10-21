pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

Rectangle {
    id: control

    width: 10
    height: control.width
    radius: control.width / 2
    color: Theme.backgroundSecondaryColor

    property alias iconPath: iconLabel.icon.source

    IconLabel {
        id: iconLabel
        anchors.centerIn: parent
        width: 10
        height: 10
        icon {
            width: 10
            height: 10
            color: Theme.primaryTextColor
        }
    }
}
