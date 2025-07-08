pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    width: 30
    height: 30

    signal clicked

    property alias emojiChar: emojiIconLabel.text
    property string tooltipText

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundOffsetHoveredColor
        visible: groupButtonHoverHandler.hovered
    }

    Label {
        id: emojiIconLabel
        anchors.fill: parent
        horizontalAlignment: Label.AlignHCenter
        verticalAlignment: Label.AlignVCenter
        font {
            family: "Noto Color Emoji"
            pixelSize: 20
        }
    }

    HoverHandler {
        id: groupButtonHoverHandler
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onSingleTapped: control.clicked()
    }

    ToolTip.visible: groupButtonHoverHandler.hovered && !!control.tooltipText
    ToolTip.text: control.tooltipText
}
