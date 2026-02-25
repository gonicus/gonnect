import QtQuick

import QtQuick.Controls.impl
import base

Rectangle {
    id: control
    color: clipboardButtonHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor
    radius: 4
    width: 24
    height: 24

    property string text

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Copy to clipboard: %1").arg(control.text)
    Accessible.focusable: true
    Accessible.onPressAction: () => ViewHelper.copyToClipboard(control.text)

    IconLabel {
        anchors.centerIn: parent
        icon {
            width: 16
            height: 16
            source: Icons.editCopy
        }

        Accessible.ignored: true
    }

    TapHandler {
        onTapped: () => ViewHelper.copyToClipboard(control.text)
    }

    HoverHandler {
        id: clipboardButtonHoverHandler
    }
}
