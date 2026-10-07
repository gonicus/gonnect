pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property alias chatRoom: chat.chatRoom
    property alias chatProvider: chat.chatProvider

    // Model for available chat rooms (e.g. when they have tagged a conference)
    property ConferenceLinkedRoomsProxyModel conferenceRoomsModel: null
    property bool showConferenceChatOption: false

    /// Emitted when the user picks an entry from the selector: -1 for the (virtual) conference
    /// chat option, otherwise the row index into conferenceRoomsModel.
    signal chatSourceSelected(int sourceIndex)

    function giveFocus() {
        chat.giveFocus()
    }

    function rebuildChatSources() {
        chatSourcesModel.clear()

        if (control.showConferenceChatOption) {
            chatSourcesModel.append({ label: qsTr("Conference chat"), sourceIndex: -1 })
        }

        const roomsModel = control.conferenceRoomsModel
        if (roomsModel) {
            for (let i = 0, l = roomsModel.rowCount(); i < l; ++i) {
                chatSourcesModel.append({ label: roomsModel.nameAt(i), sourceIndex: i })
            }
        }
    }

    onShowConferenceChatOptionChanged: () => control.rebuildChatSources()
    Component.onCompleted: () => control.rebuildChatSources()

    Connections {
        target: control.conferenceRoomsModel
        function onRowsInserted() { control.rebuildChatSources() }
        function onRowsRemoved() { control.rebuildChatSources() }
        function onModelReset() { control.rebuildChatSources() }
    }

    ListModel {
        id: chatSourcesModel
    }

    Connections {
        target: control.chatProvider
        function onClipboardImageUploaded(imageFilePath : url, chatRoom : IChatRoom, threadId : string) {
            ViewHelper.topDrawer.loader.sourceComponent = imagePreviewComponent

            const item = ViewHelper.topDrawer.loader.item
            item.source = `file://${imageFilePath}`
            item.chatRoom = chatRoom
            item.threadId = threadId
        }
    }

    Component {
        id: imagePreviewComponent

        ImageSendPreview {}
    }

    ComboBox {
        id: chatSourceSelector
        visible: chatSourcesModel.count > 1
        model: chatSourcesModel
        textRole: "label"
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: Theme.d / 2
        }

        onActivated: index => control.chatSourceSelected(chatSourcesModel.get(index).sourceIndex)
    }

    Chat {
        id: chat
        showTitleBar: false
        anchors {
            top: chatSourceSelector.visible ? chatSourceSelector.bottom : parent.top
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
    }
}
