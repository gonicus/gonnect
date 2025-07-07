pragma ComponentBehavior: Bound

import QtQuick
import base

ListView {
    id: control

    property alias chatProvider: roomModel.chatProvider
    property string selectedRoomId

    onSelectedRoomIdChanged: () => {
        if (control.selectedRoomId && control.chatProvider) {
            control.chatProvider.resetUnreadCount(control.selectedRoomId)
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
