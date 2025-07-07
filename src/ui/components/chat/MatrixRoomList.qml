pragma ComponentBehavior: Bound

import QtQuick
import base

ListView {
    id: control

    property alias chatProvider: roomModel.chatProvider
    property string selectedRoomId

    readonly property IChatRoom chatRoom: control.chatProvider && control.selectedRoomId
                                          ? control.chatProvider.chatRoomByRoomId(control.selectedRoomId)
                                          : null

    onChatRoomChanged: () => control.resetUnreadCount()
    onSelectedRoomIdChanged: () => control.resetUnreadCount()

    function resetUnreadCount() {
        if (control.selectedRoomId && control.chatRoom && control.selectedRoomId === control.chatRoom.id) {
            control.chatRoom.resetUnreadCount()
        }
    }

    model: ChatRoomProxyModel {
        ChatRoomModel {
            id: roomModel
        }
    }

    delegate: MatrixRoomListItem {
        id: delg
        highlighted: control.selectedRoomId === delg.roomId
        anchors {
            left: parent?.left
            right: parent?.right
        }

        onClicked: () => control.selectedRoomId = delg.roomId
    }
}
