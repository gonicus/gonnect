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

    onChatRoomChanged: () => {
        const room = control.chatRoom
        if (room && !room.isInitiallyLoaded) {
            room.loadMessages()
        }
    }

    AvatarImage {
        id: avatarImage
        visible: control.showTitleBar && !!control.chatRoom
        size: 30
        source: control.chatRoom?.avatarPath ?? ""
        initials: control.chatRoom ? ViewHelper.initials(control.chatRoom.name) : ""
        showPresenceStatus: !!(control.chatRoom?.hasPresenceState)
        presenceStatus: control.chatRoom?.presenceState ?? ChatUser.PresenceState.Unknown
        indicatorComponent: Component { ChatUserPresenceStatusIndicator {} }
        anchors {
            left: parent.left
            leftMargin: 10
            verticalCenter: messageListCardHeading.verticalCenter
        }
    }

    CardHeading {
        id: messageListCardHeading
        visible: titleLoadingIndicatorRow.visible || (control.showTitleBar && !!control.chatRoom)
        leftPadding: avatarImage.x + avatarImage.width - 10
        rightPadding: parent.width - favCardHeadingButton.y
        text: control.showTitleBar && control.chatRoom
              ? (control.chatRoom.isDirectChat
                 ? qsTr("Direct conversation with %1").arg(control.chatRoom.name)
                 : qsTr("Chat room %1").arg(control.chatRoom.name))
              : ""
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
    }

    Row {
        id: titleLoadingIndicatorRow
        spacing: 4
        visible: !!(control.chatRoom?.isLoadingMessageHistory && !bigLoadingItem.visible)
        anchors {
            horizontalCenter: !control.showTitleBar ? messageListCardHeading.horizontalCenter : undefined
            right: control.showTitleBar ? favCardHeadingButton.left : undefined
            rightMargin: 12
            top: messageListCardHeading.top
            bottom: messageListCardHeading.bottom
        }

        BusyIndicator {
            id: titleLoadingIndicator
            running: titleLoadingIndicatorRow.visible
            width: titleLoadingIndicator.height
            height: 24
            circleColor: Theme.secondaryTextColor
            anchors.verticalCenter: parent.verticalCenter
        }

        Label {
            text: qsTr("Messages are loading...")
            color: Theme.secondaryTextColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    FavIcon {
        id: favCardHeadingButton
        visible: control.showTitleBar && messageListCardHeading.visible
        isFavorite: control.chatRoom?.isFavorite ?? false
        anchors {
            verticalCenter: messageListCardHeading.verticalCenter
            right: messageListCardHeadingButton.left
        }

        onToggled: () => control.chatProvider?.requestToggleRoomFavorite(control.chatRoom)
    }

    CardHeadingMoreMenuButton {
        id: messageListCardHeadingButton
        visible: control.showTitleBar && messageListCardHeading.visible
        anchors {
            top: parent.top
            right: parent.right
        }

        onClicked: () => chatRoomMenuComponent.createObject(messageListCardHeadingButton).popup()
    }

    Component {
        id: chatRoomMenuComponent

        Menu {
            id: chatRoomMenu
            onClosed: () => chatRoomMenu.destroy()

            HideableMenuItem {
                text: qsTr("Edit room...")
                icon.source: Icons.editor
                visible: !!(control.chatRoom?.permissions & IChatRoom.Permission.CanEdit)
                onTriggered: () => ViewHelper.showEditRoomDialog(control.chatProvider, control.chatRoom.id)
            }
            HideableMenuItem {
                text: qsTr("Invite users...")
                icon.source: Icons.listAdd
                visible: !!(control.chatRoom?.permissions & IChatRoom.Permission.CanInvite)
                onTriggered: () => ViewHelper.showInviteUserToRoomDialog(control.chatProvider, control.chatRoom.id)
            }
            HideableMenuItem {
                text: qsTr("Leave room...")
                icon.source: Icons.dialogCancel
                onTriggered: () => {
                    const item = DialogFactory.createConfirmDialog({
                                     text: qsTr("Are you sure you really want to leave this chat?")
                                 })
                    const roomId = control.chatRoom.id
                    item.accepted.connect(() => control.chatProvider.requestRoomLeave(roomId))
                }
            }
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
            top: messageListCardHeading.visible ? messageListCardHeading.bottom : parent.top
            bottom: typingUsersList.visible
                    ? typingUsersList.top
                    : (chatMessageBox.visible
                       ? chatMessageBox.top
                       : parent.bottom)
            bottomMargin: 20
            leftMargin: 10
            rightMargin: 10
        }

        onRespondTo: messageId => relatedMsg.chatMessage = control.chatRoom?.chatMessageById(messageId) ?? null
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
                width: 40
                height: 40
                circleColor: Theme.secondaryTextColor
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
        onSendImage: imagePath => control.chatRoom.sendImage(imagePath)
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
                    chatMessageBox.text = latestMsg.content.simpleText
                    chatMessageBox.editMessageId = latestMsg.eventId
                    // ViewHelper.showEditMessageDialog(chatProvider, chatRoom.id, latestMsg.eventId, latestMsg.message)
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
        joinState: control.chatRoom?.ownUserJoinState === IChatRoom.UserRoomState.Unjoined
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
}
