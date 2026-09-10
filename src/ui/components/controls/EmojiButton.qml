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
    Accessible.description: qsTr("Selected Emoji: %1").arg(control.tooltipText)
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
        horizontalAlignment: Label.AlignHCenter
        verticalAlignment: Label.AlignVCenter
        wrapMode: Text.NoWrap
        minimumPixelSize: 10
        fontSizeMode: Text.Fit
        renderType: Text.QtRendering
        anchors {
            fill: parent
            margins: 2
        }
        font {
            family: [ "Noto Color Emoji", "Segoe UI Emoji", "Apple Color Emoji", "Twemoji Mozilla" ]
            pixelSize: 20
        }

        Accessible.ignored: true
    }

    HoverHandler {
        id: groupButtonHoverHandler
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        gesturePolicy: TapHandler.WithinBounds
        grabPermissions: PointerHandler.ApprovesTakeOverByAnything
        onSingleTapped: () => control.clicked()
    }

    ToolTip.visible: groupButtonHoverHandler.hovered && !!control.tooltipText
    ToolTip.text: control.tooltipText
}
