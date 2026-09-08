pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property IChatProvider chatProvider
    property IChatRoom chatRoom

    property bool showTitleBar: true
    readonly property alias isScrolledDown: chatMessageList.isScrolledDown

    readonly property int capabilities: control.chatProvider?.capabilities ?? 0

    function giveFocus() {
        chatMessageBox.giveFocus()
    }

    function loadMessages() {
        const room = control.chatRoom
        if (room && !room.isInitiallyLoaded && room.ownUserJoinState === IChatRoom.UserRoomState.Joined) {
            room.loadMessages()
        }
    }

    onChatRoomChanged: () => {
                           relatedMsg.chatMessage = null
                           control.loadMessages()
                       }

    Connections {
        target: control.chatRoom

        function onOwnUserJoinStateChanged() {
            control.loadMessages()
        }
    }

    ChatButtonBar {
        id: messageListCardHeading
        height: messageListCardHeading.implicitHeight
        shallBeVisible: control.showTitleBar && !!control.chatRoom
        chatProvider: control.chatProvider
        chatRoom: control.chatRoom
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
    }

    Rectangle {
        id: buttonBarBorder
        height: 1
        color: Theme.borderColor
        visible: messageListCardHeading.visible && !pinnedChatMessageList.visible
        anchors {
            left: parent.left
            right: parent.right
            top: messageListCardHeading.bottom
        }
    }

    PinnedChatMessagesList {
        id: pinnedChatMessageList
        chatRoom: control.chatRoom
        visible: pinnedChatMessageList.count > 0
        height: Math.min(pinnedChatMessageList.implicitHeight, Math.floor(parent.height * 0.15))
        z: chatMessageList.z + 1
        anchors {
            top: messageListCardHeading.visible ? messageListCardHeading.bottom : parent.top
            left: parent.left
            right: parent.right
        }
    }

    ChatMessageList {
        id: chatMessageList
        chatProvider: control.chatProvider
        chatRoom: control.chatRoom
        clip: true
        visible: control.chatRoom?.ownUserJoinState === IChatRoom.UserRoomState.Joined ?? false
        anchors {
            left: parent.left
            right: parent.right
            top: pinnedChatMessageList.visible
                 ? pinnedChatMessageList.bottom
                 : (buttonBarBorder.visible
                    ? buttonBarBorder.bottom
                    : parent.top)
            bottom: typingUsersList.visible
                    ? typingUsersList.top
                    : (chatMessageBox.visible
                       ? chatMessageBox.top
                       : parent.bottom)
            bottomMargin: 2
            leftMargin: 10
            rightMargin: 10
        }

        onRespondTo: messageId => {
                         relatedMsg.chatMessage = control.chatRoom?.chatMessageById(messageId) ?? null
                         chatMessageBox.giveFocus()
                     }
        onRetryMessage: messageId => {
                            if (control.chatProvider) {
                                control.chatProvider.retrySendMessage(control.chatRoom.id, messageId)
                            }
                        }
    }

    Item {
        id: bigLoadingItem
        visible: !!(control.chatRoom?.isLoadingMessageHistory && !chatMessageList.count)
        anchors.fill: chatMessageList

        Row {
            spacing: 20
            anchors.centerIn: parent

            BusyIndicator {
                running: bigLoadingItem.visible
                circleColor: Theme.secondaryTextColor
                contentItem.antialiasing: true
            }

            Label {
                text: qsTr("Messages are loading...")
                color: Theme.secondaryTextColor
                font.pixelSize: 22
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Item {
        id: typingUsersList
        height: 20
        anchors  {
            left: parent.left
            right: parent.right
            bottom: chatMessageBox.visible ? chatMessageBox.top : parent.bottom
        }

        readonly property list<string> typingUserNames: control.chatRoom?.typingUsers.map(user => user.computedName) ?? []

        Label {
            id: typingUsersLabel
            text: qsTr("%1 is/are typing", "", typingUsersList.typingUserNames.length).arg(typingUsersList.typingUserNames.join(", "))
            wrapMode: Label.Wrap
            color: Theme.secondaryInactiveTextColor
            font.pixelSize: 12
            visible: typingUsersList.typingUserNames.length > 0
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 10
                rightMargin: 10
                verticalCenter: parent.verticalCenter
            }
        }
    }

    Rectangle {
        id: replyBg
        color: Theme.backgroundColor
        height: relatedMsg.height
        visible: relatedMsg.visible
        topLeftRadius: 8
        topRightRadius: 8
        anchors {
            top: relatedMsg.top
            left: chatMessageBox.left
            right: chatMessageBox.right
            bottom: chatMessageBox.top
            topMargin: -10
        }

        Rectangle {
            height: 1
            color: Theme.borderColor
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
        }
    }

    ChatMessageListItemRelatedContent {
        id: relatedMsg
        visible: !!relatedMsg.chatMessage
        nickName: relatedMsg.chatMessage?.nickName ?? ""
        isStateUpdate: relatedMsg.chatMessage?.isStateUpdate?? false
        content: relatedMsg.chatMessage?.content ?? null
        userState: relatedMsg.chatMessage?.state ?? ChatMessageContentUserStateChange.State.Unknown
        affectedUserName: control.chatProvider?.userById(relatedMsg.chatMessage?.affectedUserId ?? "")?.computedName ?? ""
        anchors {
            left: replyBg.left
            right: replyBg.right
            leftMargin: 10
            bottom: chatMessageBox.top
            bottomMargin: 10
        }

        property ChatMessage chatMessage
    }

    HeaderIconButton {
        id: closeButton
        visible: relatedMsg.visible
        iconSource: Icons.mobileCloseApp
        anchors {
            top: replyBg.top
            right: replyBg.right
            topMargin: 10
            rightMargin: 10
        }

        onClicked: () => relatedMsg.chatMessage = null
    }

    ChatMessageBox {
        id: chatMessageBox
        visible: control.chatRoom?.ownUserJoinState === IChatRoom.UserRoomState.Joined ?? false
        chatRoom: control.chatRoom
        capabilities: control.capabilities
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        onSendFile: filePath => control.chatRoom.sendFile(filePath)
        onImageFromClipboardReceived: () => {
            if (control.chatProvider && control.chatRoom) {
                control.chatProvider.uploadImageFromClipboard(control.chatRoom.id)
            }
        }
        onEditLastMessage: () => {
            const chatProvider = control.chatProvider
            const chatRoom = control.chatRoom
            if (chatRoom && chatProvider) {
                const latestMsg = control.chatRoom.latestOwnTextMessage()
                if (latestMsg) {
                    chatMessageBox.text = latestMsg.content.rawText
                    chatMessageBox.editMessageId = latestMsg.eventId
                    chatMessageBox.positionCursorAtEnd()
                }
            }
        }
        onSendMessage: () => {
            if (chatMessageBox.hasMessage) {

                if (chatMessageBox.editMessageId) {
                    // Edit existing message
                    control.chatProvider.requestEditMessage(control.chatRoom.id, chatMessageBox.editMessageId, chatMessageBox.text)
                    chatMessageBox.editMessageId = ""
                } else {
                    // Send new message
                    control.chatRoom.sendMessage(chatMessageBox.text,
                                                         relatedMsg.chatMessage ? relatedMsg.chatMessage.eventId : "")
                }

                relatedMsg.chatMessage = null
                chatMessageBox.clear()
            }
        }
    }

    ChatUnjoinedPage {
        id: chatUnjoinedPage
        chatProvider: control.chatProvider
        chatRoom: control.chatRoom
        joinState: control.chatRoom?.ownUserJoinState ?? IChatRoom.UserRoomState.Unjoined
        visible: control.chatRoom?.ownUserJoinState !== IChatRoom.UserRoomState.Joined ?? false
        anchors {
            left: parent.left
            right: parent.right
            top: messageListCardHeading.visible ? messageListCardHeading.bottom : parent.top
            bottom: parent.bottom
            leftMargin: 10
            rightMargin: 10
        }
    }

    Timer {
        id: readTimer
        interval: 2000
        onTriggered: () => {
            if (control.Window.active && control.isScrolledDown && control.chatRoom) {
                control.chatRoom.resetUnreadCount()
            }
        }
    }

    HoverHandler {
        id: chatHoverHandler
        onPointChanged: () => {
            if (!readTimer.running && control.Window.active) {
                readTimer.start()
            }
        }
    }

    FileDropArea {
        anchors.fill: parent
        onDropAccepted: urls => ViewHelper.showFileUploadDialog(control.chatRoom, urls)
    }
}
