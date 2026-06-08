pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

Item {
    id: control
    width: 36
    anchors {
        top: parent?.top
        bottom: parent?.bottom
    }

    signal clicked

    property alias icon: buttonIcon.icon.source

    IconLabel {
        id: buttonIcon
        anchors.centerIn: parent
        icon {
            width: 22
            height: 22
            color: control.enabled
                   ? (hoverHandler.hovered
                      ? Theme.primaryTextColor
                      : Theme.secondaryTextColor)
                   : Theme.secondaryInactiveTextColor
        }
    }

    HoverHandler {
        id: hoverHandler
    }

    TapHandler {
        onTapped: () => {
            if (control.enabled) {
                control.clicked()
            }
        }
    }
}
