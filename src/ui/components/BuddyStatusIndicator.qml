pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import QtQuick.Layouts
import base

/// Colored circle showing the status of a SIP buddy
Rectangle {
    id: control

    width: 10
    height: control.width
    radius: control.width / 2
    color: 'transparent'
    border.width: 0
    border.color: Theme.borderColor

    /// The status to show (as in SIPBuddyState enum)
    property int status: SIPBuddyState.UNKNOWN

    property bool isBlocked: false

    SequentialAnimation on color {
        id: ringingAnimation
        running: false
        loops: Animation.Infinite

        ColorAnimation { from: 'transparent'; to: Theme.greenColor; duration: 1000 }
        ColorAnimation { from: Theme.greenColor; to: 'transparent'; duration: 1000 }
    }

    IconLabel {
        id: blockedIcon
        visible: false
        height: 10
        width: 10
        anchors.centerIn: parent
        icon {
            width: 10
            height: 10
            source: Icons.dialogCancel
        }
    }

    states: [
        State {
            when: control.isBlocked

            PropertyChanges {
                control.width: 12
                control.height: 12
                control.color: Theme.backgroundColor
                blockedIcon.visible: true
            }
        },
        State {
            when: control.status === SIPBuddyState.UNKNOWN

            PropertyChanges {
                control.border.width: 1
            }
        },
        State {
            when: control.status === SIPBuddyState.UNAVAILABLE

            PropertyChanges {
                control.color: Theme.borderColor
            }
        },
        State {
            when: control.status === SIPBuddyState.READY

            PropertyChanges {
                control.color: Theme.greenColor
            }
        },
        State {
            when: control.status === SIPBuddyState.RINGING

            PropertyChanges {
                ringingAnimation.running: true
            }
        },
        State {
            when: control.status === SIPBuddyState.BUSY

            PropertyChanges {
                control.color: Theme.redColor
            }
        }
    ]
}
