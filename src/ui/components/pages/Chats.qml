pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property IChatProvider attachedData

    readonly property IChatRoom selectedChatRoom: control.attachedData && chatRoomList.selectedRoomId
                                                  ? control.attachedData.chatRoomByRoomId(chatRoomList.selectedRoomId)
                                                  : null

    function showChatRoom(roomId : string) {
        console.debug(category, `Showing room "${roomId}" on chats page`)
        chatRoomList.selectRoom(roomId)
    }

    function useImageFromClipboard() {
        if (control.selectedChatRoom) {
            control.attachedData.uploadImageFromClipboard(chatRoomList.selectedRoomId)
        }
    }

    Timer {
        id: showChatRoomTimer
        interval: 100

        property string roomId

        onRoomIdChanged: () => showChatRoomTimer.start()

        onTriggered: () => {
            ViewHelper.showChatRoom(control.attachedData, showChatRoomTimer.roomId)
        }
    }

    Connections {
        id: chatProviderConnections
        target: control.attachedData
        function onChatRoomAdded(index : int, room : IChatRoom, tag : string) {
            if (tag) {
                showChatRoomTimer.roomId = room.id
            }
        }

        function onChatRoomLeft(roomId : string, roomName : string, leaveReason : int, message : string) {
            let reasonStr = ""

            switch (leaveReason) {
                case IChatRoom.LeaveReason.Unknown :
                    reasonStr = qsTr("You have left room '%1' for an unknown reason.").arg(roomName)
                    break
                case IChatRoom.LeaveReason.User:
                    reasonStr = qsTr("You have successfully left room '%1'.").arg(roomName)
                    break
                case IChatRoom.LeaveReason.Kicked:
                    reasonStr = qsTr("You have been kicked out room '%1'.").arg(roomName)
                    break
                case IChatRoom.LeaveReason.Banned:
                    reasonStr = qsTr("You have been banned from '%1'.").arg(roomName)
                    break
                default:
                    throw new Error("Unknown enum value for leaveReason:", leaveReason)
            }

            const msg = message.trim()
            if (msg !== "") {
                reasonStr += '\n\n\n'
                reasonStr += qsTr("Message from the causing user:")
                reasonStr += `\n\n${message}`
            }

            DialogFactory.createInfoDialog({text : reasonStr })
        }

        function onRoomInviteReceived(roomId : string, roomDisplayName : string, invitationText : string) {
            ViewHelper.topDrawer.loader.sourceComponent = roomInvitedComponent

            const item = ViewHelper.topDrawer.loader.item
            item.chatProvider = control.attachedData
            item.roomId = roomId
            item.roomDisplayName = roomDisplayName
            item.invitationText = invitationText
        }

        function onClipboardImageUploaded(imageFilePath : url, chatRoom : IChatRoom) {
            ViewHelper.topDrawer.loader.sourceComponent = imagePreviewComponent

            const item = ViewHelper.topDrawer.loader.item
            item.source = `file://${imageFilePath}`
            item.chatRoom = chatRoom
        }
    }

    Component {
        id: roomInvitedComponent

        InvitedToChatRoom {}
    }

    Component {
        id: imagePreviewComponent

        ImageSendPreview {}
    }

    LoggingCategory {
        id: category
        name: "gonnect.qml.Chats"
        defaultLogLevel: LoggingCategory.Warning
    }

    states: [
        State {
            when: !control.attachedData || !control.attachedData.isConnected

            PropertyChanges {
                sideBar.visible: false
                chatMainCard.visible: false
                connectingCard.visible: true
            }
        },
        State {
            when: control.attachedData
                  && control.attachedData.hasDeviceVerification
                  && !control.attachedData.isDeviceVerified

            PropertyChanges {
                sideBar.visible: false
                chatMainCard.visible: false
                verificationCard.visible: true
            }
        },
        State {
            when: true

            PropertyChanges {
                sideBar.visible: true
                chatMainCard.visible: true
            }
        }
    ]

    Card {
        id: connectingCard
        visible: false
        anchors {
            fill: parent
            leftMargin: 24
            rightMargin: 24
            bottomMargin: 24
        }

        BusyIndicator {
            running: connectingCard.visible
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: connectingCard.verticalCenter
                bottomMargin: 24
            }
        }

        Label {
            text: qsTr("Connecting...")
            font.pixelSize: 22
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.verticalCenter
                topMargin: 24
            }
        }
    }

    Card {
        id: verificationCard
        visible: false
        anchors {
            fill: parent
            leftMargin: 24
            rightMargin: 24
            bottomMargin: 24
        }

        DeviceVerification {
            anchors.fill: parent
            chatProvider: control.attachedData
        }
    }

    Card {
        id: sideBar
        width: control.width * 1 / 4
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            leftMargin: 24
            rightMargin: 24
            bottomMargin: 24
        }

        Item {
            id: roomListCardHeading
            height: 46

            anchors {
                left: parent.left
                right: parent.right
            }

            Rectangle {
                color: Theme.borderColor
                height: 1
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
            }
        }

        CardHeadingMoreMenuButton {
            id: sortCardHeadingButton
            iconSource: Icons.viewSort
            width: 22
            anchors {
                top: parent.top
                right: roomListCardHeadingButton.left
                rightMargin: 6
            }

            onClicked: () => sortMenuComponent.createObject(sortCardHeadingButton).open()
        }

        Component {
            id: sortMenuComponent

            Popup {
                y: sortCardHeadingButton.height
                contentHeight: sortCol.implicitHeight
                contentWidth: Math.max(
                                  sortButtonGroup.buttons.reduce((acc, val) => Math.max(acc, val.implicitWidth), 0),
                                  unreadOnTopCheckBox.implicitWidth + unreadOnTopCheckBox.anchors.leftMargin
                                                                    + unreadOnTopCheckBox.anchors.rightMargin)

                ButtonGroup {
                    id: sortButtonGroup
                }

                Column {
                    id: sortCol
                    spacing: 10
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Column {
                        id: radioButtonCol
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Repeater {
                            model: [
                                ChatRoomProxyModel.SortStrategy.Alphabetical,
                                ChatRoomProxyModel.SortStrategy.LatestActivity,
                            ]
                            delegate: RadioButton {
                                id: sortDelg
                                text: EnumTranslation.chatRoomSortStrategy(sortDelg.modelData)
                                checked: chatRoomList.sortStrategy === sortDelg.modelData
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }

                                required property int modelData

                                ButtonGroup.group: sortButtonGroup

                                onToggled: () => {
                                    sortSettings.setValue("chatRoomSortStrategy", sortDelg.modelData)
                                    chatRoomList.sortStrategy = sortDelg.modelData
                                }
                            }
                        }
                    }

                    Rectangle {
                        color: Theme.borderColor
                        height: 1
                        anchors {
                            left: parent.left
                            right: parent.right
                        }
                    }

                    CheckBox {
                        id: favoritesOnTopCheckBox
                        text: qsTr("Show favorites on top")
                        checked: sortSettings.showFavoriteRoomsOnTop
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: 9
                            rightMargin: 9
                        }

                        onToggled: () => {
                            sortSettings.showFavoriteRoomsOnTop = favoritesOnTopCheckBox.checked
                        }
                    }

                    CheckBox {
                        id: unreadOnTopCheckBox
                        text: qsTr("Show unread chats on top")
                        checked: sortSettings.showUnreadRoomsInOwnGroup
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: 9
                            rightMargin: 9
                        }

                        onToggled: () => {
                            sortSettings.showUnreadRoomsInOwnGroup = unreadOnTopCheckBox.checked
                        }
                    }
                }
            }
        }

        CardHeadingMoreMenuButton {
            id: roomListCardHeadingButton
            width: 22
            anchors {
                top: parent.top
                right: parent.right
                rightMargin: 12
            }

            onClicked: () => roomListMenuComponent.createObject(roomListCardHeadingButton).popup()
        }

        Component {
            id: roomListMenuComponent

            Menu {
                Action {
                    text: qsTr("Search user...")
                    icon.source: Icons.systemSearch
                    onTriggered: () => ViewHelper.showChatUserSearchDialog(control.attachedData)
                }
                Action {
                    text: qsTr("Search public room...")
                    icon.source: Icons.systemSearch
                    onTriggered: () => ViewHelper.showPublicRoomSearchDialog(control.attachedData)
                }
                Action {
                    text: qsTr("Create room...")
                    icon.source: Icons.listAdd
                    onTriggered: () => ViewHelper.showCreateRoomDialog(control.attachedData, [])
                }
            }
        }

        Settings {
            id: sortSettings
            location: ViewHelper.userConfigPath
            category: "generic"

            property bool showUnreadRoomsInOwnGroup
            property bool showFavoriteRoomsOnTop: true

            Component.onCompleted: () => {
                chatRoomList.sortStrategy = parseInt(sortSettings.value("chatRoomSortStrategy",
                                                                        ChatRoomProxyModel.SortStrategy.Alphabetical), 10)
            }
        }

        Flickable {
            id: roomFlickable
            contentHeight: roomCol.implicitHeight
            clip: true
            anchors {
                top: roomListCardHeading.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                leftMargin: 10
            }

            ScrollBar.vertical: ScrollBar { width: 5 }

            Column {
                id: roomCol
                topPadding: 10
                anchors {
                    left: parent.left
                    right: parent.right
                    rightMargin: 10
                }

                ChatRoomListSectionHeader {
                    text: qsTr("Unread")
                    visible: unreadRoomList.count > 0
                    anchors {
                        left: parent.left
                        right: parent.right
                        leftMargin: 10
                        rightMargin: 10
                    }
                }

                ChatRoomList {
                    id: unreadRoomList
                    chatProvider: control.attachedData ?? null
                    selectedRoomId: chatRoomList.selectedRoomId
                    onlyUnread: true
                    active: sortSettings.showUnreadRoomsInOwnGroup
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    onRoomSelected: (roomId) => chatRoomList.selectRoom(roomId)
                }

                ChatRoomList {
                    id: chatRoomList
                    chatProvider: control.attachedData ?? null
                    groupFavorites: sortSettings.showFavoriteRoomsOnTop
                    showSectionHeader: sortSettings.showFavoriteRoomsOnTop || unreadRoomList.count > 0
                    hasFavorites: control.attachedData?.hasFavoriteRooms ?? false
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    onRoomSelected: (roomId) => chatRoomList.selectRoom(roomId)
                    onSelectedRoomIdChanged: () => chatRoomList.resetUnreadCount()
                    onChatRoomChanged: () => {
                        chatRoomList.resetUnreadCount()

                        if (chatRoomList.chatRoom && !chatRoomList.chatRoom.isInitiallyLoaded) {
                            chatRoomList.chatRoom.loadMessages()
                        }
                    }

                    readonly property IChatRoom chatRoom: chatRoomList.chatProvider && chatRoomList.selectedRoomId
                                                          ? chatRoomList.chatProvider.chatRoomByRoomId(chatRoomList.selectedRoomId)
                                                          : null

                    function resetUnreadCount() {
                        if (chatRoomList.selectedRoomId
                                && chatRoomList.chatRoom
                                && chatRoomList.selectedRoomId === chatRoomList.chatRoom.id
                                && chatRoomList.selectedListItem?.ownJoinState === IChatRoom.UserRoomState.Joined) {

                            chatRoomList.chatRoom.resetUnreadCount()
                        }
                    }

                    function selectRoom(roomId : string) {
                        chatRoomList.selectedRoomId = roomId

                        // Find index of item to scroll ListView such that it is visible
                        if (roomId) {

                            const l = chatRoomList.count
                            for (let i = 0; i < l; ++i) {
                                const item = chatRoomList.itemAt(i)
                                if (item.roomId === roomId) {

                                    // Check if flicking is needed
                                    const currMinY = roomFlickable.contentY
                                    const currMaxY = roomFlickable.contentY + roomFlickable.height

                                    let newY = 0
                                    if (item.y < roomFlickable.contentY) {
                                        // Item is too high - scroll so it is the top-most visible item
                                        newY = item.y
                                    } else if (item.y + item.height > roomFlickable.contentY + roomFlickable.height) {
                                        // Item is too low - scroll so it is the bottom-most visible item
                                        newY = item.y + item.height - roomFlickable.height
                                    } else {
                                        return
                                    }

                                    // Flick to item; clamp for min/max values
                                    const maxY = roomCol.implicitHeight - roomFlickable.height
                                    roomFlickable.contentY = Util.clamp(newY, 0, maxY)

                                    return
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Card {
        id: chatMainCard
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
            left: sideBar.right
            leftMargin: 24
            rightMargin: 24
            bottomMargin: 24
        }

        Timer {
            id: readTimer
            interval: 2000
            onTriggered: () => {
                if (control.Window.active && chatMessageList.isScrolledDown) {
                    chatRoomList.resetUnreadCount()
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

        AvatarImage {
            id: avatarImage
            visible: !!chatMessageList.chatRoom
            size: 30
            source: chatMessageList.chatRoom?.avatarPath ?? ""
            initials: chatMessageList.chatRoom ? ViewHelper.initials(chatMessageList.chatRoom.name) : ""
            showPresenceStatus: !!(chatRoomList.chatRoom?.hasPresenceState)
            presenceStatus: chatRoomList.chatRoom?.presenceState ?? ChatUser.PresenceState.Unknown
            indicatorComponent: Component { ChatUserPresenceStatusIndicator {} }
            anchors {
                left: parent.left
                leftMargin: 10
                verticalCenter: messageListCardHeading.verticalCenter
            }
        }

        CardHeading {
            id: messageListCardHeading
            visible: !!chatMessageList.chatRoom
            leftPadding: avatarImage.x + avatarImage.width - 10
            rightPadding: parent.width - favCardHeadingButton.y
            text: chatMessageList.chatRoom
                  ? (chatMessageList.chatRoom.isDirectChat
                     ? qsTr("Direct conversation with %1").arg(chatMessageList.chatRoom.name)
                     : qsTr("Chat room %1").arg(chatMessageList.chatRoom.name))
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
            visible: !!(chatRoomList.chatRoom?.isLoadingMessageHistory && !bigLoadingItem.visible)
            anchors {
                right: favCardHeadingButton.left
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
            visible: messageListCardHeading.visible
            isFavorite: chatMessageList.chatRoom?.isFavorite ?? false
            anchors {
                verticalCenter: messageListCardHeading.verticalCenter
                right: messageListCardHeadingButton.left
            }

            onToggled: () => chatRoomList.chatProvider?.requestToggleRoomFavorite(chatMessageList.chatRoom)
        }

        CardHeadingMoreMenuButton {
            id: messageListCardHeadingButton
            visible: messageListCardHeading.visible
            anchors {
                top: parent.top
                right: parent.right
            }

            onClicked: () => chatRoomMenuComponent.createObject(messageListCardHeadingButton).popup()
        }

        Component {
            id: chatRoomMenuComponent

            Menu {
                Action {
                    text: qsTr("Edit room...")
                    icon.source: Icons.editor
                    enabled: !!(control.selectedChatRoom?.permissions & IChatRoom.Permission.CanEdit)
                    onTriggered: () => ViewHelper.showEditRoomDialog(control.attachedData, chatRoomList.selectedRoomId)
                }
                Action {
                    text: qsTr("Invite users...")
                    icon.source: Icons.listAdd
                    enabled: !!(control.selectedChatRoom?.permissions & IChatRoom.Permission.CanInvite)
                    onTriggered: () => ViewHelper.showInviteUserToRoomDialog(control.attachedData, chatRoomList.selectedRoomId)
                }
                Action {
                    text: qsTr("Leave room...")
                    icon.source: Icons.dialogCancel
                    onTriggered: () => {
                        const item = DialogFactory.createConfirmDialog({
                                         text: qsTr("Are you sure you really want to leave this chat?")
                                     })
                        const roomId = chatRoomList.selectedRoomId
                        item.accepted.connect(() => control.attachedData.requestRoomLeave(roomId))
                    }
                }
            }
        }

        ChatMessageList {
            id: chatMessageList
            chatProvider: control.attachedData
            chatRoom: chatRoomList.chatRoom
            clip: true
            visible: chatRoomList.chatRoom?.ownUserJoinState === IChatRoom.UserRoomState.Joined ?? false
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

            onRespondTo: messageId => relatedMsg.chatMessage = chatRoomList.chatRoom?.chatMessageById(messageId) ?? null
        }

        Item {
            id: bigLoadingItem
            visible: !!(chatRoomList.chatRoom?.isLoadingMessageHistory && !chatMessageList.count)
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

            readonly property list<string> typingUserNames: chatRoomList.selectedListItem?.typingUserNames ?? []

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
            visible: chatRoomList.chatRoom?.ownUserJoinState === IChatRoom.UserRoomState.Joined ?? false
            chatRoom: chatMessageList.chatRoom
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            onSendFile: filePath => chatMessageList.chatRoom.sendFile(filePath)
            onSendImage: imagePath => chatMessageList.chatRoom.sendImage(imagePath)
            onImageFromClipboardReceived: () => control.useImageFromClipboard()
            onEditLastMessage: () => {
                const chatProvider = control.attachedData
                const chatRoom = chatRoomList.chatRoom
                if (chatRoom && chatProvider) {
                    const latestMsg = chatRoomList.chatRoom.latestOwnTextMessage()
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
                        chatRoomList.chatProvider.requestEditMessage(chatRoomList.chatRoom.id, chatMessageBox.editMessageId, chatMessageBox.text)
                        chatMessageBox.editMessageId = ""
                    } else {
                        // Send new message
                        chatMessageList.chatRoom.sendMessage(chatMessageBox.text,
                                                             relatedMsg.chatMessage ? relatedMsg.chatMessage.eventId : "")
                    }

                    relatedMsg.chatMessage = null
                    chatMessageBox.clear()
                }
            }
        }

        ChatUnjoinedPage {
            id: chatUnjoinedPage
            chatProvider: control.attachedData
            chatRoom: chatRoomList.chatRoom
            joinState: chatRoomList.chatRoom?.ownUserJoinState === IChatRoom.UserRoomState.Unjoined
            visible: chatRoomList.chatRoom?.ownUserJoinState !== IChatRoom.UserRoomState.Joined ?? false
            anchors {
                left: parent.left
                right: parent.right
                top: messageListCardHeading.visible ? messageListCardHeading.bottom : parent.top
                bottom: parent.bottom
                leftMargin: 10
                rightMargin: 10
            }
        }
    }
}
