pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Item {
    id: control

    property alias chatRoom: chatModel.chatRoom
    property IChatProvider chatProvider

    readonly property alias isScrolledDown: listView.atYEnd
    readonly property alias count: listView.count

    signal respondTo(string messageId)

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
    }

    ListView {
        id: listView
        spacing: 5
        verticalLayoutDirection: ListView.BottomToTop
        anchors.fill: parent
        model: ChatProxyModel {
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
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property int index

            onRespondTo: messageId => control.respondTo(messageId)
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
                       && !control.chatRoom.isCompletelyLoaded) {

                // Load next batch from history
                control.chatRoom.loadMessages()
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
