pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as B
import base

B.Switch {
    id: control

    indicator: Rectangle {
        implicitWidth: 36
        implicitHeight: 20
        x: control.leftPadding
        y: parent?.height / 2 - height / 2
        radius: 13
        color: control.checked ? Theme.accentColor : Theme.backgroundColor
        border.color: control.checked ? Theme.accentColor : Theme.borderColor

        Behavior on color { ColorAnimation { duration: 100 } }
        Behavior on border.color { ColorAnimation { duration: 100 } }

        Rectangle {
            x: control.checked ? (parent.width - width - 2) : 2
            y: parent.height / 2 - height / 2
            width: 16
            height: width
            radius: width / 2
            color: control.checked ? Theme.backgroundColor : Theme.secondaryTextColor

            Behavior on x { NumberAnimation { duration: 100 } }
            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    contentItem: Label {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
}
