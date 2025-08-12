pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    signal callClicked
    signal callAsClicked(string id)
    signal notifyWhenAvailableClicked
    signal blockTemporarilyClicked

    property bool favoriteAvailable: true

    property string phoneNumber
    property bool isFavorite
    property bool isAnonymous
    property bool isReady
    property bool isBlocked
    property bool isSipSubscriptable

    Action {
        text: qsTr('Call')
        onTriggered: () => control.callClicked()
    }
    Action {
        text: qsTr('Copy number')
        onTriggered: () => ViewHelper.copyToClipboard(control.phoneNumber)
    }
    Action {
        id: favToggleAction
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        enabled: !control.isAnonymous
        onTriggered: () => ViewHelper.toggleFavorite(control.phoneNumber, NumberStats.ContactType.PhoneNumber)
    }
    Action {
        text: qsTr('Remind when available')
        enabled: control.isSipSubscriptable && !control.isReady
        onTriggered: () => control.notifyWhenAvailableClicked()
    }
    Action {
        text: control.isBlocked ? qsTr('Unblock') : qsTr('Block for 8 hours')
        enabled: !control.isAnonymous
        onTriggered: () => control.blockTemporarilyClicked()
    }
}
