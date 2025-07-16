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
        text: qsTr('Start conference')
        enabled: !ViewHelper.isActiveVideoCall
        onTriggered: () => control.callClicked()
    }
    Action {
        id: favToggleAction
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        onTriggered: () => ViewHelper.toggleFavorite(control.roomName, NumberStats.ContactType.JitsiMeetUrl)
    }
    Action {
        text: qsTr('Copy room name')
        onTriggered: () => ViewHelper.copyToClipboard(control.roomName)
    }
}
