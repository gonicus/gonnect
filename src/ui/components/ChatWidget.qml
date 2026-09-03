pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWidget {
    id: control
    minCellWidth: 20
    minCellHeight: 15

    property IChatProvider chatProvider
    property IChatRoom chatRoom

    // Unread counts are already aggregated via ChatConnectorManager.unreadNotificationsCount,
    // so reporting them here would double count them in the global badge.
    notifications: 0

    onAdditionalSettingsLoaded: () => control.resolveRoom()

    property bool hasRoomConfig: false

    function providerById(providerId : string) : IChatProvider {
        for (const provider of ChatConnectorManager.chatConnectors) {
            if (provider.id === providerId) {
                return provider
            }
        }
        return null
    }

    // The chat room is only stored by the ids of its provider and itself, because the
    // underlying objects are re-created on restarts or reconnects.
    function resolveRoom() {
        const roomConfig = control.config.get("room").toString()
        const separatorIndex = roomConfig.indexOf("|")

        control.hasRoomConfig = separatorIndex >= 0
        if (separatorIndex < 0) {
            control.chatProvider = null
            control.chatRoom = null
            return
        }

        const providerId = roomConfig.substring(0, separatorIndex)
        const roomId = roomConfig.substring(separatorIndex + 1)
        const provider = control.providerById(providerId)
        control.chatProvider = provider
        control.chatRoom = provider ? provider.chatRoomByRoomId(roomId) : null
    }

    Connections {
        target: ChatConnectorManager

        function onChatConnectorsChanged() {
            // Providers may only become available after this widget has been created
            control.resolveRoom()
        }

        function onIsChatAvailableChanged() {
            control.resolveRoom()
        }
    }

    Connections {
        target: control.chatProvider

        function onIsConnectedChanged() {
            // Rooms may have been re-created on a reconnect
            control.resolveRoom()
        }

        function onChatRoomAdded(index : int, room : IChatRoom) {
            // The rooms are only loaded asynchronously after the provider has connected
            control.resolveRoom()
        }

        function onChatRoomRemoved(index : int, room : IChatRoom) {
            if (room === control.chatRoom) {
                control.chatRoom = null
            }
        }

        function onChatRoomLeft(roomId : string, roomName : string, leaveReason : int, message : string) {
            if (control.chatRoom && roomId === control.chatRoom.id) {
                control.chatRoom = null
            }
        }
    }

    Rectangle {
        id: chatWidget
        parent: control.root
        color: "transparent"
        anchors.fill: parent

        Chat {
            id: chat
            visible: !!control.chatRoom
            chatProvider: control.chatProvider
            chatRoom: control.chatRoom
            anchors.fill: parent
        }

        Label {
            id: chatInfo
            visible: !control.chatRoom
            text: control.hasRoomConfig
                  ? qsTr("Chat room not available")
                  : qsTr("No chat room selected")
            wrapMode: Label.Wrap
            color: Theme.secondaryTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.centerIn: parent

            Accessible.role: Accessible.StaticText
            Accessible.name: chatInfo.text
            Accessible.description: qsTr("Displays the current status of the widget: %1").arg(chatInfo.text)
        }
    }
}