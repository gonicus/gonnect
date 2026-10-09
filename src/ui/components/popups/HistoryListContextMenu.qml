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
    }

    HideableMenuItem {
        id: chatAction
        text: qsTr('Chat')
        icon.source: Icons.dialogMessages
        visible: control.isOpenChatAvailable
        onTriggered: () => control.chatClicked()
    }

    HideableMenuItem {
        id: copyAction
        text: qsTr('Copy number')
        icon.source: Icons.editCopy
        onTriggered: () => ClipboardHelper.copyToClipboard(control.phoneNumber)
    }

    HideableMenuItem {
        id: favToggleAction
        text: control.isFavorite ? qsTr('Remove favorite') : qsTr('Add favorite')
        icon.source: Icons.folderFavorites
        visible: !control.isAnonymous
        onTriggered: () => ViewHelper.toggleFavorite(control.phoneNumber, NumberStats.ContactType.PhoneNumber)
    }

    HideableMenuItem {
        id: remindAction
        text: qsTr('Remind when available')
        icon.source: Icons.notifications
        visible: control.isSipSubscriptable && !control.isReady
        onTriggered: () => control.notifyWhenAvailableClicked()
    }

    HideableMenuItem {
        id: blockAction
        text: control.isBlocked ? qsTr('Unblock') : qsTr('Block for 8 hours')
        icon.source: Icons.dialogCancel
        visible: !control.isAnonymous
        onTriggered: () => control.blockTemporarilyClicked()
    }

    Action {
        id: removeAction
        text: qsTr("Remove")
        icon.source: Icons.userTrash
        onTriggered: () => control.removeItem()
    }
}
