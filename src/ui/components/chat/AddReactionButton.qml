pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitWidth: 24
    implicitHeight: 24

    signal clicked

    IconLabel {
        id: emojiIconLabel
        icon {
            source: Icons.smileyAdd
            color: hoverHandler.hovered ? Theme.primaryTextColor : Theme.secondaryTextColor
            width: 20
            height: 20
        }
        anchors {
            centerIn: parent
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
