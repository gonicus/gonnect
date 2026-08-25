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

    readonly property real percentageRead: control.readUsers.length / Math.max(control.allUsersCount - 1, 1) * 100

    IconLabel {
        anchors.centerIn: parent
        width: control.implicitWidth
        height: control.implicitHeight
        icon {
            width: (control.implicitWidth * ScreenInfo.devicePixelRatio) | 0
            height: (control.implicitHeight * ScreenInfo.devicePixelRatio) | 0
            source: {
                const percentage = control.percentageRead
                if (percentage < 1) {
                    return Icons.taskProcess0
                }
                if (percentage < 33) {
                    return Icons.taskProcess1
                }
                if (percentage < 66) {
                    return Icons.taskProcess2
                }
                if (percentage < 99) {
                    return Icons.taskProcess3
                }
                return Icons.taskProcess4
            }
        }
    }

    HoverHandler {
        id: readMarkerHoverHandler
    }

    ToolTip.visible: readMarkerHoverHandler.hovered
    ToolTip.text: qsTr("%1 of %2 have read this message:\n%3")
                    .arg(control.readUsers.length)
                    .arg(Math.max(control.allUsersCount - 1, 1))
                    .arg(control.readUsers.map(u => u.computedName).join(', '))
}
