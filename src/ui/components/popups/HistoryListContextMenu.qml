pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    signal callClicked
    signal notifyWhenAvailableClicked
    signal blockTemporarilyClicked

    property string phoneNumber
    property bool isFavorite
    property bool isAnonymous
    property bool isBusy
    property bool isBlocked

    Action {
        text: qsTr('Call')
        onTriggered: () => control.callClicked()
    }
    Action {
        text: qsTr('Copy number')
        onTriggered: () => ViewHelper.copyToClipboard(control.phoneNumber)
    }
    Action {
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        enabled: !control.isAnonymous
        onTriggered: () => ViewHelper.toggleFavorite(control.phoneNumber)
    }
    Action {
        text: qsTr('Remind when available')
        enabled: control.isBusy
        onTriggered: () => control.notifyWhenAvailableClicked()
    }
    Action {
        text: control.isBlocked ? qsTr('Unblock') : qsTr('Block for 8 hours')
        enabled: !control.isAnonymous
        onTriggered: () => control.blockTemporarilyClicked()
    }
}
