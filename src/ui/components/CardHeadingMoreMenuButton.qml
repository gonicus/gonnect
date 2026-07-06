pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

Item {
    id: control
    width: 46
    height: 46

    signal clicked

    property alias iconLabel: iconLabel.text
    property alias iconSource: iconLabel.icon.source

    IconLabel {
        id: iconLabel
        anchors.centerIn: parent
        font.pixelSize: 22
        icon {
            width: 22
            height: 22
            source: Icons.overflowMenu
            color: control.enabled
                   ? (buttonHoverHandler.hovered
                      ? Theme.primaryTextColor
                      : Theme.secondaryTextColor)
                   : Theme.secondaryInactiveTextColor
        }
    }

    HoverHandler {
        id: buttonHoverHandler
    }

    TapHandler {
        onTapped: () => {
            if (control.enabled) {
                control.clicked()
            }
        }
    }
}
