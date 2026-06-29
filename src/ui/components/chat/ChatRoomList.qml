pragma ComponentBehavior: Bound

import QtQuick
import base

Repeater {
    id: control

    property alias chatProvider: roomModel.chatProvider
    property alias filterText: proxyModel.filterText
    property alias sortStrategy: proxyModel.sortStrategy
    property alias onlyUnread: proxyModel.onlyUnread
    property alias groupFavorites: proxyModel.groupFavorites

    property bool active: true
    property bool showSectionHeader: false
    property bool hasFavorites: false

    readonly property alias selectedListItem: internal.selectedListItem

    signal roomSelected(string roomId)

    onHasFavoritesChanged: () => internal.sectionHeaderUpdateRequired()
    onItemAdded: () => internal.sectionHeaderUpdateRequired()
    onItemRemoved: () => internal.sectionHeaderUpdateRequired()

    readonly property QtObject _internal: QtObject {
        id: internal

        signal sectionHeaderUpdateRequired

        property ChatRoomListItem selectedListItem

        function updateSelectedListItem() {
            Qt.callLater(() => {
                const selectedRoom = SelectionState.selectedChatRoom
                if (!selectedRoom) {
                    internal.selectedListItem = null
                }

                const l = control.count
                for (let i = 0; i < l; ++i) {
                    const item = control.itemAt(i)
                    if (item.roomId === selectedRoom.id) {
                        internal.selectedListItem = item
                        return
                    }
                }

                internal.selectedListItem = null
            })
        }

        readonly property Connections selectionStateConnections: Connections {
            target: SelectionState

            function onSelectedChatRoomChanged() {
                internal.updateSelectedListItem()
            }
        }
    }

    readonly property ChatRoomProxyModel chatRoomProxyModel: ChatRoomProxyModel {
        id: proxyModel

        ChatRoomModel {
            id: roomModel
        }
    }

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Chat room list")
    Accessible.description: qsTr("List of all chat rooms")

    model: control.active ? proxyModel : null

    delegate: ChatRoomListItem {
        id: delg
        highlighted: SelectionState.selectedChatRoom?.id === delg.roomId
        anchors {
            left: parent?.left
            right: parent?.right
        }

        onHighlightedChanged: () => internal.updateSelectedListItem()

        onClicked: () => control.roomSelected(delg.roomId)
        onFileDropped: url => control.chatProvider.chatRoomByRoomId(delg.roomId).sendFile(url)
        onFavoriteToggled: () => control.chatProvider.requestToggleRoomFavorite(
                                     control.chatProvider.chatRoomByRoomId(delg.roomId))
        onEditRoomTriggered: () => ViewHelper.showEditRoomDialog(control.chatProvider, delg.roomId)
        onInviteUsersTriggered: () => ViewHelper.showInviteUserToRoomDialog(control.chatProvider, delg.roomId)
        onLeaveRoomTriggered: () => {
            const item = DialogFactory.createConfirmDialog({
                             text: qsTr("Are you sure you really want to leave this chat?")
                         })
            const roomId = delg.roomId
            item.accepted.connect(() => {
                                      if (SelectionState.selectedChatRoom?.id === roomId) {
                                          control.roomSelected("")
                                      }

                                      control.chatProvider.requestRoomLeave(roomId)
                                  })

        }
    }
}
