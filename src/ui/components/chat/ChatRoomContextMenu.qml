pragma ComponentBehavior: Bound

import QtQuick
import base

Menu {
    id: control
    onClosed: () => control.destroy()

    signal favoriteToggled
    signal leaveRoomTriggered
    signal editRoomTriggered
    signal inviteUsersTriggered
    signal editConferenceUrlTriggered

    property bool toggleFavoriteVisible: true
    property bool editRoomVisible: true
    property bool inviteUsersVisible: true
    property bool editConferenceUrlVisible: false

    HideableMenuItem {
        visible: control.toggleFavoriteVisible
        text: qsTr("Toggle favorite")
        icon.source: Icons.folderFavorites
        onTriggered: () => control.favoriteToggled()
    }

    HideableMenuItem {
        visible: control.editRoomVisible
        text: qsTr("Edit room...")
        icon.source: Icons.editor
        onTriggered: () => control.editRoomTriggered()
    }

    HideableMenuItem {
        visible: control.inviteUsersVisible
        text: qsTr("Invite users...")
        icon.source: Icons.listAdd
        onTriggered: () => control.inviteUsersTriggered()
    }

    HideableMenuItem {
        visible: control.editConferenceUrlVisible
        text: qsTr("Edit conference url...")
        icon.source: Icons.videoCall
        onTriggered: () => control.editConferenceUrlTriggered()
    }

    HideableMenuItem {
        text: qsTr("Leave room...")
        icon.source: Icons.dialogCancel
        onTriggered: () => control.leaveRoomTriggered()
    }
}
