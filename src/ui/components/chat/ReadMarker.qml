pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import base

Item {
    id: control

    implicitWidth: Theme.d * 2
    implicitHeight: control.implicitWidth

    property var readUsers
    property int allUsersCount

    readonly property int readUsersCount: control.readUsers?.length ?? 0

    IconLabel {
        anchors.centerIn: parent
        width: control.implicitWidth
        height: control.implicitHeight
        icon {
            width: Math.round(control.implicitWidth * Screen.devicePixelRatio)
            height: Math.round(control.implicitHeight * Screen.devicePixelRatio)
            source: {
                if (control.readUsersCount === control.allUsersCount - 1) {
                    return Icons.readmarkDoubleFilled
                }
                if (control.readUsersCount > 0) {
                    return Icons.readmarkDoubleInline
                }
                return Icons.readmarkSingleInline
            }
        }
    }

    HoverHandler {
        id: readMarkerHoverHandler
    }

    ToolTip.visible: readMarkerHoverHandler.hovered && control.readUsersCount > 0
    ToolTip.text: qsTr("%1 of %2 have read this message:\n%3")
                    .arg(control.readUsersCount)
                    .arg(Math.max(control.allUsersCount - 1, 1))
                    .arg((control.readUsers ?? []).map(u => u.computedName).join(', '))
}
