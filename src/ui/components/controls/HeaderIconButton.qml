pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitWidth: 28
    implicitHeight: 28

    property alias iconSource: iconItem.icon.source
    property int iconSize: 14
    property alias active: hoverHandler.enabled

    property string accessiblePurpose

    signal clicked

    Accessible.role: Accessible.Button
    Accessible.name: control.accessiblePurpose
    Accessible.focusable: true
    Accessible.onPressAction: () => control.clicked()

    Rectangle {
        visible: hoverHandler.hovered
        anchors.fill: parent
        radius: parent.width / 2
        color: Theme.backgroundHeaderIconHovered
        border.width: 1
        border.color: Theme.borderHeaderIconHovered

        Accessible.ignored: true
    }

    IconLabel {
        id: iconItem
        width: control.iconSize
        height: control.iconSize
        anchors.centerIn: parent
        icon {
            width: control.iconSize
            height: control.iconSize
            color: control.active ? Theme.foregroundHeaderIcons : Theme.foregroundHeaderIconsInactive
        }

        Accessible.ignored: true
    }

    HoverHandler {
        id: hoverHandler
    }

    TapHandler {
        enabled: control.enabled
        onTapped: () => control.clicked()
    }
}
