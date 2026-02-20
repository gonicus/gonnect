pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    signal callClicked

    property string roomName
    property bool isFavorite

    Action {
        id: startAction
        text: qsTr('Start conference')
        enabled: !ViewHelper.isActiveVideoCall
        onTriggered: () => control.callClicked()

        Accessible.role: Accessible.MenuItem
        Accessible.name: startAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.callClicked()
    }

    Action {
        id: favToggleAction
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        onTriggered: () => ViewHelper.toggleFavorite(control.roomName, NumberStats.ContactType.JitsiMeetUrl)

        Accessible.role: Accessible.MenuItem
        Accessible.name: favToggleAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.toggleFavorite(control.roomName, NumberStats.ContactType.JitsiMeetUrl)
    }

    Action {
        id: copyAction
        text: qsTr('Copy room name')
        onTriggered: () => ViewHelper.copyToClipboard(control.roomName)

        Accessible.role: Accessible.MenuItem
        Accessible.name: copyAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.copyToClipboard(control.roomName)
    }
}
