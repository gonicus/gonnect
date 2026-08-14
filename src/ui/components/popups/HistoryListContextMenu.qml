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

    onClosed: () => control.destroy()

    HideableMenuItem {
        id: callAction
        text: qsTr('Call')
        icon.source: Icons.callStart
        onTriggered: () => control.callClicked()

        Accessible.role: Accessible.Button
        Accessible.name: callAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.callClicked()
    }

    HideableMenuItem {
        id: copyAction
        text: qsTr('Copy number')
        icon.source: Icons.editCopy
        onTriggered: () => ClipboardHelper.copyToClipboard(control.phoneNumber)

        Accessible.role: Accessible.Button
        Accessible.name: copyAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ClipboardHelper.copyToClipboard(control.phoneNumber)
    }

    HideableMenuItem {
        id: favToggleAction
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        icon.source: Icons.folderFavorites
        visible: !control.isAnonymous
        onTriggered: () => ViewHelper.toggleFavorite(control.phoneNumber, NumberStats.ContactType.PhoneNumber)

        Accessible.role: Accessible.Button
        Accessible.name: favToggleAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.toggleFavorite(control.phoneNumber, NumberStats.ContactType.PhoneNumber)
    }

    HideableMenuItem {
        id: remindAction
        text: qsTr('Remind when available')
        icon.source: Icons.notifications
        visible: control.isSipSubscriptable && !control.isReady
        onTriggered: () => control.notifyWhenAvailableClicked()

        Accessible.role: Accessible.Button
        Accessible.name: remindAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.notifyWhenAvailableClicked()
    }

    HideableMenuItem {
        id: blockAction
        text: control.isBlocked ? qsTr('Unblock') : qsTr('Block for 8 hours')
        icon.source: Icons.dialogCancel
        visible: !control.isAnonymous
        onTriggered: () => control.blockTemporarilyClicked()

        Accessible.role: Accessible.Button
        Accessible.name: blockAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.blockTemporarilyClicked()
    }
}
