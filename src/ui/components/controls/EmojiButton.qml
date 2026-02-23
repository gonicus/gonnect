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

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Emoji")
    Accessible.description: qsTr("Selected emoji: ") + control.tooltipText
    Accessible.focusable: true
    Accessible.onPressAction: () => control.clicked()

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundOffsetHoveredColor
        visible: groupButtonHoverHandler.hovered

        Accessible.ignored: true
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

        Accessible.ignored: true
    }

    HoverHandler {
        id: groupButtonHoverHandler
        cursorShape: Qt.PointingHandCursor

        Accessible.ignored: true
    }

    TapHandler {
        onSingleTapped: control.clicked()

        Accessible.ignored: true
    }

    ToolTip.visible: groupButtonHoverHandler.hovered && !!control.tooltipText
    ToolTip.text: control.tooltipText
}
