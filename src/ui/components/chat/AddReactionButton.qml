pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitWidth: 24
    implicitHeight: 24

    signal clicked

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 8
        border.width: 1
        border.color: hoverHandler.hovered
                      ? Theme.borderHeaderIconHovered
                      : Theme.borderColor
        color: hoverHandler.hovered
               ? Theme.backgroundOffsetHoveredColor
               : "transparent"
    }

    Label {
        id: emojiIconLabel
        horizontalAlignment: Label.AlignHCenter
        verticalAlignment: Label.AlignVCenter
        color: hoverHandler.hovered ? Theme.primaryTextColor : Theme.secondaryTextColor
        text: "+"
        anchors {
            centerIn: parent
            verticalCenterOffset: 1
        }
        font {
            family: "Noto Color Emoji"
            pixelSize: 16
        }
    }

    TapHandler {
        id: tapHandler
        onTapped: () => control.clicked()
    }

    HoverHandler {
        id: hoverHandler
        cursorShape: Qt.PointingHandCursor
    }
}
