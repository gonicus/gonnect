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
        id: callAction
        text: qsTr('Call')
        onTriggered: () => control.callClicked()

        Accessible.role: Accessible.Button
        Accessible.name: callAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.callClicked()
    }

    Action {
        id: copyAction
        text: qsTr('Copy number')
        onTriggered: () => ViewHelper.copyToClipboard(control.phoneNumber)

        Accessible.role: Accessible.Button
        Accessible.name: copyAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.copyToClipboard(control.phoneNumber)
    }

    Action {
        id: favToggleAction
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        enabled: !control.isAnonymous
        onTriggered: () => ViewHelper.toggleFavorite(control.phoneNumber, NumberStats.ContactType.PhoneNumber)

        Accessible.role: Accessible.Button
        Accessible.name: favToggleAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.toggleFavorite(control.phoneNumber, NumberStats.ContactType.PhoneNumber)
    }

    Action {
        id: remindAction
        text: qsTr('Remind when available')
        enabled: control.isSipSubscriptable && !control.isReady
        onTriggered: () => control.notifyWhenAvailableClicked()

        Accessible.role: Accessible.Button
        Accessible.name: remindAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.notifyWhenAvailableClicked()
    }

    Action {
        id: blockAction
        text: control.isBlocked ? qsTr('Unblock') : qsTr('Block for 8 hours')
        enabled: !control.isAnonymous
        onTriggered: () => control.blockTemporarilyClicked()

        Accessible.role: Accessible.Button
        Accessible.name: blockAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.blockTemporarilyClicked()
    }
}
