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

    onClosed: () => control.destroy()

    Action {
        id: startAction
        text: qsTr('Start conference')
        enabled: !VideoCallHelper.hasActiveVideoCall
        onTriggered: () => control.callClicked()
    }

    Action {
        id: favToggleAction
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        onTriggered: () => ViewHelper.toggleFavorite(control.roomName, NumberStats.ContactType.JitsiMeetUrl)
    }

    Action {
        id: copyAction
        text: qsTr('Copy room name')
        onTriggered: () => ClipboardHelper.copyToClipboard(control.roomName)
    }

    Action {
        id: removeAction
        text: qsTr("Remove")
        icon.source: Icons.userTrash
        onTriggered: () => control.removeItem()
    }
}
