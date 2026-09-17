pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

/// Colored circle showing the status of a chat user
Rectangle {
    id: control

    width: 10
    height: control.width
    radius: control.width / 2
    color: 'transparent'
    border.width: 0
    border.color: Theme.borderColor

    /// The status to show (as in ChatUser::PresenceState enum)
    property int status: ChatUser.PresenceState.Unknown

    states: [
        State {
            when: control.status === ChatUser.PresenceState.Unknown

            PropertyChanges {
                control.border.width: 1
            }
        },
        State {
            when: control.status === ChatUser.PresenceState.Offline

            PropertyChanges {
                control.color: Theme.secondaryInactiveTextColor
            }
        },
        State {
            when: control.status === ChatUser.PresenceState.Away

            PropertyChanges {
                control.color: Theme.yellowColor
            }
        },
        State {
            when: control.status === ChatUser.PresenceState.Online

            PropertyChanges {
                control.color: Theme.greenColor
            }
        }
    ]
}
