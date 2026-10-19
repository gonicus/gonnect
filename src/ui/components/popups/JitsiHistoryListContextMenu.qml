pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    signal callClicked
    signal removeItem

    property string roomName
    property bool isFavorite
    property bool showRemove: false

    onClosed: () => control.destroy()

    HideableMenuItem {
        id: startAction
        text: qsTr('Start conference')
        enabled: !VideoCallHelper.hasActiveVideoCall
        onTriggered: () => control.callClicked()
    }

    HideableMenuItem {
        id: favToggleAction
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        onTriggered: () => ViewHelper.toggleFavorite(control.roomName, NumberStats.ContactType.JitsiMeetUrl)
    }

    HideableMenuItem {
        id: copyAction
        text: qsTr('Copy room name')
        onTriggered: () => ClipboardHelper.copyToClipboard(control.roomName)
    }

    HideableMenuItem {
        id: removeAction
        text: qsTr("Remove")
        visible: control.showRemove
        icon.source: Icons.userTrash
        onTriggered: () => control.removeItem()
    }
}
