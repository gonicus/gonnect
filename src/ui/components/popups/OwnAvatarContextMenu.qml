pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    onClosed: () => control.destroy()

    LoggingCategory {
        id: category
        name: "gonnect.qml.popup.OwnAvatarContextMenu"
        defaultLogLevel: LoggingCategory.Info
    }

    function setPresenceState(presenceState : int) {
        GlobalStateAggregator.presenceState = presenceState
        console.log(category, "User setting presence state to", presenceState)
    }

    function openStatusTextEditPopup() {
        ViewHelper.showStatusTextEditDialog()
    }

    MenuItem {
        id: dndAction
        text: qsTr("Do not disturb")
        icon {
            source: Icons.imUserBusy
            color: "transparent"
        }
        onTriggered: () => control.setPresenceState(PresenceState.Busy)

        Accessible.role: Accessible.Button
        Accessible.name: dndAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.setPresenceState(PresenceState.Busy)
    }

    MenuItem {
        id: awayAction
        text: qsTr("Away")
        icon {
            source: Icons.imUserAway
            color: "transparent"
        }
        onTriggered: () => control.setPresenceState(PresenceState.Away)

        Accessible.role: Accessible.Button
        Accessible.name: awayAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.setPresenceState(PresenceState.Away)
    }

    MenuItem {
        id: availableAction
        text: qsTr("Available")
        icon {
            source: Icons.imUserOnline
            color: "transparent"
        }
        onTriggered: () => control.setPresenceState(PresenceState.Available)

        Accessible.role: Accessible.Button
        Accessible.name: availableAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.setPresenceState(PresenceState.Available)
    }

    MenuSeparator { }

    MenuItem {
        id: setStatusTextAction
        text: qsTr("Set status text...")
        icon.source: Icons.editor
        onTriggered: () => control.openStatusTextEditPopup()

        Accessible.role: Accessible.Button
        Accessible.name: setStatusTextAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.openStatusTextEditPopup()
    }
}
