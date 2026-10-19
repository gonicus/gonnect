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
    readonly property bool isThreadMode: SelectionState.selectedThreadId !== ""

    signal respondTo(string messageId)
    signal respondInNewThread(string messageId)
    signal retryMessage(string messageId)

    onChatRoomChanged: () => {
        internal.autoScrollBottom = true
        listView.positionViewAtBeginning()
        internal.startPendingJump()
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
        property string jumpTargetMessageId: ""

        onJumpTargetMessageIdChanged: () => {
                                          if (internal.jumpTargetMessageId !== "") {
                                              internal.jumpBusyShowTimer.restart()
                                          } else {
                                              internal.jumpBusyShowTimer.stop()
                                              jumpBusyOverlay.visible = false
                                          }
                                      }

        readonly property Timer jumpBusyShowTimer: Timer {
            interval: 200
            onTriggered: () => jumpBusyOverlay.visible = true
        }

        readonly property Connections controlConnections: Connections {
            target: control
            function onChatRoomChanged() { internal.updateIsCompletelyLoaded() }
        }

        readonly property Connections selectionStateConnections: Connections {
            target: SelectionState
            function onSelectedThreadIdChanged() { internal.updateIsCompletelyLoaded() }
            function onMessageJumpInfoChanged() { internal.startPendingJump() }
        }

        readonly property Connections chatRoomConnections: Connections {
            target: control.chatRoom
            function onIsCompletelyLoadedChanged() { internal.updateIsCompletelyLoaded() }
            function onIsLoadingMessageHistoryChanged() {
                if (internal.jumpTargetMessageId !== "" && !control.chatRoom.isLoadingMessageHistory) {
                    internal.continueJump()
                }
            }
        }

        readonly property Timer highlightClearTimer: Timer {
            interval: 2200
            onTriggered: () => SelectionState.highlightedMessageId = ""
        }

        function updateIsCompletelyLoaded() {
            internal.isCompletelyLoaded = control.chatRoom?.isCompletelyLoaded(SelectionState.selectedThreadId) ?? false
        }

        function startPendingJump() {
            const jump = SelectionState.messageJumpInfo
            if (jump.isEmpty() || !control.chatRoom || jump.roomId !== control.chatRoom.id) {
                return
            }

            internal.jumpTargetMessageId = jump.messageId
            SelectionState.messageJumpInfo = { roomId: "", messageId: "" }
            internal.continueJump()
        }

        function continueJump() {
            if (internal.jumpTargetMessageId === "") {
                return
            }

            // Target message loaded and rendered
            const filteredRow = chatProxyModel.rowOfMessage(internal.jumpTargetMessageId)
            if (filteredRow >= 0) {
                listView.positionViewAtIndex(filteredRow, ListView.Center)
                SelectionState.highlightedMessageId = internal.jumpTargetMessageId
                internal.jumpTargetMessageId = ""
                internal.highlightClearTimer.restart()
                return
            }

            // Target message in time line, but filtered by proxy model
            const rawRow = chatModel.rowOfMessage(internal.jumpTargetMessageId)
            if (rawRow >= 0) {
                const msg = control.chatRoom.messageByIndex(rawRow)
                if (msg.threadId !== SelectionState.selectedThreadId) {
                    SelectionState.selectedThreadId = msg.threadId
                }
                Qt.callLater(internal.continueJump)
                return
            }

            // All loaded, but target not findable
            if (internal.isCompletelyLoaded) {
                internal.jumpTargetMessageId = ""
                return
            }

            // Target message not loaded yet - load it
            if (control.chatRoom && !control.chatRoom.isLoadingMessageHistory) {
                const oldestId = control.chatRoom.chatMessageCount() > 0
                               ? control.chatRoom.messageByIndex(0).eventId
                               : ""

                control.chatRoom.loadMessages(SelectionState.selectedThreadId, oldestId)
            }
        }
    }


    ListView {
        id: listView
        spacing: 5
        verticalLayoutDirection: ListView.BottomToTop
        anchors.fill: parent
        model: ChatProxyModel {
            id: chatProxyModel
            threadId: SelectionState.selectedThreadId

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
            chatRoom: control.chatRoom
            roomPermissions: control.chatRoom?.permissions ?? 0
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property int index

            onRespondTo: messageId => control.respondTo(messageId)
            onRespondInNewThread: threadId => control.respondInNewThread(threadId)
            onRetryMessage: messageId => control.retryMessage(messageId)
            onOpenThread: threadId => SelectionState.selectedThreadId = threadId
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
                control.chatRoom.loadMessages(SelectionState.selectedThreadId)
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
        }

        Label {
            text: "↓"
            anchors.centerIn: parent
            font.pixelSize: Theme.fontSizeLarge

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

    Item {
        id: jumpBusyOverlay
        visible: false
        anchors.fill: parent
        z: 10

        Rectangle {
            anchors.fill: parent
            color: Theme.backgroundColor
            opacity: 0.6
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            cursorShape: Qt.WaitCursor
            onWheel: wheel => wheel.accepted = true
        }

        Column {
            anchors.centerIn: parent
            spacing: Theme.d

            BusyIndicator {
                running: jumpBusyOverlay.visible
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.d * 4
                height: Theme.d * 4
                circleColor: Theme.primaryTextColor
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Loading messages...")
                color: Theme.primaryTextColor
            }
        }
    }
}
