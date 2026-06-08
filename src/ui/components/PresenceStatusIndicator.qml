pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

/// Colored circle showing a presence status
Rectangle {
    id: control

    width: 10
    height: control.width
    radius: control.width / 2
    color: 'transparent'
    border.width: 0
    border.color: Theme.borderColor

    /// The status to show (as in PresenceState)
    property int status: PresenceState.Unknown

    property bool isBlocked: false
    property bool isUnregistered: false

    Accessible.ignored: true

    SequentialAnimation {
        id: ringingAnimation
        running: false
        loops: Animation.Infinite

        ColorAnimation {
            target: control
            property: "color"
            from: 'transparent'
            to: Theme.greenColor
            duration: 1000
        }
        ColorAnimation {
            target: control
            property: "color"
            from: Theme.greenColor
            to: 'transparent'
            duration: 1000
        }
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

        Accessible.ignored: true
    }

    states: [
        State {
            when: control.isUnregistered

            PropertyChanges {
                control.color: Theme.borderColor
            }
        },
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
            when: control.status === PresenceState.Unknown

            PropertyChanges {
                control.border.width: 1
            }
        },
        State {
            when: control.status === PresenceState.Offline

            PropertyChanges {
                control.color: Theme.borderColor
            }
        },
        State {
            when: control.status === PresenceState.Available

            PropertyChanges {
                control.color: Theme.greenColor
            }
        },
        State {
            when: control.status === PresenceState.Ringing

            PropertyChanges {
                ringingAnimation.running: true
            }

        },
        State {
            when: control.status === PresenceState.Away

            PropertyChanges {
                control.color: Theme.yellowColor
            }
        },
        State {
            when: control.status === PresenceState.Busy

            PropertyChanges {
                control.color: Theme.redColor
            }
        }
    ]
}
