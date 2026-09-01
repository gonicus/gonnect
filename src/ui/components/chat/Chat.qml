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

    readonly property Contact soleOtherContact: control.chatRoom && control.chatRoom.isDirectChat
                                                ? ContactHelper.lookupByChatUser(control.chatRoom.otherUser)
                                                : null

    readonly property real headerRightLimitX: titleLoadingIndicatorRow.visible
            ? titleLoadingIndicatorRow.x
            : (favCardHeadingButton.visible
               ? favCardHeadingButton.x
               : (messageListCardHeadingButton.visible
                  ? messageListCardHeadingButton.x
                  : parent.width))

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
        rightPadding: callContactButton.visible
                      ? parent.width - (control.headerRightLimitX - callContactButton.implicitWidth - Theme.d * 2) - 20
                      : parent.width - control.headerRightLimitX - 20
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

    Button {
        id: callContactButton
        icon.source: Icons.callStart
        text: qsTr("Call")
        width: callContactButton.implicitWidth
        leftPadding: Theme.d / 2
        rightPadding: Theme.d
        spacing: Theme.d / 2
        icon.width: Theme.d * 2
        icon.height: Theme.d * 2
        topInset: Theme.d * 0.68
        bottomInset: Theme.d * 0.68
        x: Math.min(messageListCardHeading.textEndX + Theme.d,
                    control.headerRightLimitX - callContactButton.width - Theme.d)
        anchors {
            top: messageListCardHeading.top
            bottom: messageListCardHeading.bottom
        }
        visible: false // control.showTitleBar && control.soleOtherContact

        onClicked: () => {
                       const soleNumber = control.numbersModel.soleNumber()
                       if (soleNumber !== "") {
                           SIPCallManager.call(soleNumber)
                       } else {
                           const item = phoneNumbersMenuComponent.createObject(callContactButton, { contact: control.soleOtherContact })
                           if (!item) {
                               console.error("Error on creating phone numbers menu")
                           }
                           item.popup()
                           item.updateWidth()
                       }
                   }
    }

    readonly property PhoneNumbersModel numbersModel: PhoneNumbersModel {
        contact: control.soleOtherContact
    }

    Component {
        id: phoneNumbersMenuComponent

        Menu {
            id: phoneNumberMenu
            onClosed: () => phoneNumberMenu.destroy()

            required property Contact contact

            function updateWidth() {
                let w = 0
                for (let i = 0, l = phoneNumberMenu.count; i < l; ++i) {
                    const item = phoneNumberMenu.itemAt(i)
                    w = Math.max(w, item.contentItem.implicitWidth + item.padding * 2)
                }
                phoneNumberMenu.width = w
            }

            Instantiator {
                model: control.numbersModel
                delegate: MenuItem {
                    id: menuDelg
                    text: PhoneNumberUtil.tooltipText(menuDelg.addr, phoneNumberMenu.contact?.computedName ?? "")
                    icon.source: PhoneNumberUtil.iconSource(menuDelg.addr)

                    required property string number
                    required property int type

                    readonly property var addr: ({
                                                     addr: menuDelg.number,
                                                     numberType: menuDelg.type,
                                                     contactType: NumberStats.ContactType.PhoneNumber
                                                 })

                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Call contact button")
                    Accessible.description: qsTr("Selected number %1").arg(menuDelg.number)
                    Accessible.focusable: true
                    Accessible.onPressAction: () => PhoneNumberUtil.startMeetingOrCall(menuDelg.addr)

                    onTriggered: () => SIPCallManager.call(menuDelg.number)
                }

                onObjectAdded: (index, object) => {
                                   phoneNumberMenu.insertItem(index, object)
                                   phoneNumberMenu.updateWidth()
                               }
                onObjectRemoved: (index, object) => {
                                     phoneNumberMenu.removeItem(object)
                                     phoneNumberMenu.updateWidth()
                                 }
            }
        }
    }

    Row {
        id: titleLoadingIndicatorRow
        spacing: 4
        visible: !!(control.chatRoom?.isLoadingMessageHistory && !bigLoadingItem.visible)
        anchors {
            horizontalCenter: !control.showTitleBar ? messageListCardHeading.horizontalCenter : undefined
            right: control.showTitleBar ? favCardHeadingButton.left : undefined
            rightMargin: Theme.d
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

        onClicked: () => {
                       chatRoomMenuComponent.createObject(messageListCardHeadingButton, {
                                                              toggleFavoriteVisible: false,
                                                              editRoomVisible: !!(Number(control.chatRoom?.permissions ?? 0) & IChatRoom.Permission.CanEdit),
                                                              inviteUsersVisible: !!(Number(control.chatRoom?.permissions ?? 0) & IChatRoom.Permission.CanInvite)
                                                          }).popup()
                   }
    }

    Component {
        id: chatRoomMenuComponent

        ChatRoomContextMenu {
            onEditRoomTriggered: () => ViewHelper.showEditRoomDialog(control.chatProvider, control.chatRoom.id)
            onInviteUsersTriggered: () => ViewHelper.showInviteUserToRoomDialog(control.chatProvider, control.chatRoom.id)
            onLeaveRoomTriggered: () => {
                                      const item = DialogFactory.createConfirmDialog({
                                                       text: qsTr("Are you sure you really want to leave this chat?")
                                                   })
                                      const roomId = control.chatRoom.id
                                      const chatProvider = control.chatProvider
                                      item.accepted.connect(() => chatProvider.requestRoomLeave(roomId))
                                  }
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
                 : (messageListCardHeading.visible
                    ? messageListCardHeading.bottom
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
