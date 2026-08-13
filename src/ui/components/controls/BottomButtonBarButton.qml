pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
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

    property string toolTipText

    ToolTip.text: control.toolTipText
    ToolTip.visible: control.enabled && hoverHandler.hovered && !!control.toolTipText.trim()

    IconLabel {
        id: buttonIcon
        anchors.centerIn: parent
        icon {
            width: 22
            height: 22
            color: control.enabled
                   ? (hoverHandler.hovered
                      ? Theme.primaryTextColor
                      : (Theme.isDarkMode ? Theme.secondaryTextColor : Theme.foregroundHeaderIcons))
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
