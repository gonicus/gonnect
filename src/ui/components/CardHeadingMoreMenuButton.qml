pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    width: 46
    height: 46

    signal clicked

    property alias iconLabel: iconLabel.text
    property alias iconSource: iconLabel.icon.source
    property int iconSize: Theme.d * 2

    // Must be done via binding because IconLabel.icon properties do not update on bindings
    Binding {
        target: iconLabel
        property: "width"
        value: control.iconSize
    }

    Binding {
        target: iconLabel
        property: "height"
        value: control.iconSize
    }

    IconLabel {
        id: iconLabel
        anchors.centerIn: parent
        width: control.iconSize
        height: control.iconSize
        font.pixelSize: control.iconSize
        icon {
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
