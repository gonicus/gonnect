pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitWidth: 40
    implicitHeight: 24

    property alias emojiText: emojiIconLabel.text
    property alias countText: countLabel.text
    property bool ownSelected: false

    signal clicked

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 8
        border.width: control.ownSelected ? 2 : 1
        border.color: hoverHandler.hovered
                      ? Theme.borderHeaderIconHovered
                      : (control.ownSelected
                         ? Theme.highlightColor
                         : Theme.borderColor)
        color: hoverHandler.hovered
               ? Theme.backgroundOffsetHoveredColor
               : (control.ownSelected
                  ? Theme.backgroundHeaderIconHovered
                  : "transparent")
    }

    Label {
        id: emojiIconLabel
        horizontalAlignment: Label.AlignHCenter
        verticalAlignment: Label.AlignVCenter
        font {
            family: "Noto Color Emoji"
            pixelSize: 14
        }

        anchors {
            left: parent.left
            leftMargin: 5
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: 1
        }
    }

    Label {
        id: countLabel
        horizontalAlignment: Label.AlignHCenter
        verticalAlignment: Label.AlignVCenter
        color: (control.ownSelected || hoverHandler.hovered) ? Theme.primaryTextColor : Theme.secondaryTextColor
        font.pixelSize: 12
        anchors {
            right: parent.right
            rightMargin: 7
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: 1
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
