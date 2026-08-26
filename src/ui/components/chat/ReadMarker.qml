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
    readonly property real percentageRead: control.readUsersCount / Math.max(control.allUsersCount - 1, 1) * 100
    readonly property int thresholdLow: 1
    readonly property int thresholdMid1: 33
    readonly property int thresholdMid2: 66
    readonly property int thresholdHigh: 99

    IconLabel {
        anchors.centerIn: parent
        width: control.implicitWidth
        height: control.implicitHeight
        icon {
            width: Math.round(control.implicitWidth * Screen.devicePixelRatio)
            height: Math.round(control.implicitHeight * Screen.devicePixelRatio)
            source: {
                const pct = control.percentageRead
                if (pct < control.thresholdLow) {
                    return Icons.taskProcess0
                }
                if (pct < control.thresholdMid1) {
                    return Icons.taskProcess1
                }
                if (pct < control.thresholdMid2) {
                    return Icons.taskProcess2
                }
                if (pct < control.thresholdHigh) {
                    return Icons.taskProcess3
                }
                return Icons.taskProcess4
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
