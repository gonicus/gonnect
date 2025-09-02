pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitHeight: 54

    required property string roomId
    required property string name
    required property int unreadCount

    property alias highlighted: selectedBackground.visible

    signal clicked

    Rectangle {
        id: selectedBackground
        visible: false
        color: Theme.backgroundOffsetHoveredColor
        radius: 4
        anchors {
            fill: parent
            leftMargin: 10
            rightMargin: 10
        }
    }

    Rectangle {
        id: hoverBackground
        visible: hoverHandler.hovered
        color: Theme.backgroundOffsetHoveredColor
        radius: 4
        anchors {
            fill: parent
            leftMargin: 10
            rightMargin: 10
        }
    }

    Label {
        id: nameLabel
        font.weight: control.unreadCount ? Font.DemiBold : Font.Normal
        text: control.unreadCount
              ? `${control.name} (${control.unreadCount})`
              : control.name
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            leftMargin: 20
            rightMargin: 20
        }
    }

    HoverHandler {
        id: hoverHandler
    }

    TapHandler {
        onTapped: () => control.clicked()
    }
}
