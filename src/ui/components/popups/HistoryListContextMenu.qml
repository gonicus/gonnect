pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    signal callClicked
    signal chatClicked
    signal callAsClicked(string id)
    signal notifyWhenAvailableClicked
    signal blockTemporarilyClicked
    signal removeItem

    property bool favoriteAvailable: true

    property string phoneNumber
    property bool isFavorite
    property bool isAnonymous
    property bool isReady
    property bool isBlocked
    property bool isSipSubscriptable
    property bool isOpenChatAvailable

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
        id: chatAction
        text: qsTr('Chat')
        icon.source: Icons.dialogMessages
        visible: control.isOpenChatAvailable
        onTriggered: () => control.chatClicked()

        Accessible.role: Accessible.Button
        Accessible.name: chatAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.chatClicked()
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

    Action {
        id: removeAction
        text: qsTr("Remove")
        onTriggered: () => control.removeItem()

        Accessible.role: Accessible.Button
        Accessible.name: removeAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => control.removeItem()
    }
}
