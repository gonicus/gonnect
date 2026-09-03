pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Item {
    id: control

    property alias chatRoom: chatModel.chatRoom
    property IChatProvider chatProvider
    property alias threadId: chatProxyModel.threadId

    readonly property alias isScrolledDown: listView.atYEnd
    readonly property alias count: listView.count
    readonly property bool isThreadMode: chatProxyModel.threadId !== ""

    signal respondTo(string messageId)
    signal retryMessage(string messageId)

    onChatRoomChanged: () => {
        internal.autoScrollBottom = true
        listView.positionViewAtBeginning()
    }

    Connections {
        target: listView.model
        function onModelReset() {
            if (internal.autoScrollBottom) {
                Qt.callLater(listView.positionViewAtBeginning)
            }
        }
    }

    QtObject {
        id: internal

        property bool autoScrollBottom: true
        property bool isCompletelyLoaded: false

        readonly property Connections controlConnections: Connections {
            target: control
            function onThreadIdChanged() { internal.updateIsCompletelyLoaded() }
            function onChatRoomChanged() { internal.updateIsCompletelyLoaded() }
        }

        readonly property Connections chatRoomConnections: Connections {
            target: control.chatRoom
            function onIsCompletelyLoadedChanged() { internal.updateIsCompletelyLoaded() }
        }

        function updateIsCompletelyLoaded() {
            internal.isCompletelyLoaded = control.chatRoom?.isCompletelyLoaded(control.threadId) ?? false
        }
    }

    ListView {
        id: listView
        spacing: 5
        verticalLayoutDirection: ListView.BottomToTop
        anchors.fill: parent
        model: ChatProxyModel {
            id: chatProxyModel

            ChatModel {
                id: chatModel
            }
        }

        Accessible.role: Accessible.List
        Accessible.name: qsTr("Chat message list")
        Accessible.description: qsTr("List of all chat messages of the current chat room")

        delegate: ChatMessageListItem {
            id: delg
            chatProvider: control.chatProvider
            roomPermissions: control.chatRoom?.permissions ?? 0
            isThreadMode: control.isThreadMode
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property int index

            onRespondTo: messageId => control.respondTo(messageId)
            onRetryMessage: messageId => control.retryMessage(messageId)
            onOpenThread: threadId => control.threadId = threadId
            onTogglePin: () => control.chatRoom?.togglePin(delg.eventId)
        }

        onMovementStarted: () => {
            internal.autoScrollBottom = false
        }

        onMovementEnded: () => {
            if (listView.atYEnd) {
                internal.autoScrollBottom = true
            } else if (listView.atYBeginning
                       && control.chatRoom
                       && control.chatRoom.isInitiallyLoaded
                       && !control.chatRoom.isLoadingMessageHistory
                       && !internal.isCompletelyLoaded) {

                // Load next batch from history
                control.chatRoom.loadMessages(control.threadId)
            }
        }
    }

    Item {
        id: autoScrollDownButton
        visible: !internal.autoScrollBottom
        width: 50
        height: autoScrollDownButton.width
        anchors {
            bottom: parent.bottom
            right: parent.right
        }

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Auto scroll down")
        Accessible.focusable: true
        Accessible.onPressAction: () => autoScrollDownButton.scrollAction()

        Rectangle {
            id: autoScrollBackground
            anchors.fill: parent
            radius: parent.width / 2
            color: autoScrollHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor

            Accessible.ignored: true
        }

        Label {
            text: "↓"
            anchors.centerIn: parent
            font.pixelSize: 20

            Accessible.ignored: true
        }

        HoverHandler {
            id: autoScrollHoverHandler
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            id: autoScrollTapHandler
            onTapped: () => autoScrollDownButton.scrollAction()
        }

        function scrollAction() {
            internal.autoScrollBottom = true
            listView.positionViewAtBeginning()
        }
    }
}
