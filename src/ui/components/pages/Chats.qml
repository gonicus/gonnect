pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property IChatProvider attachedData

    function showChatRoom(roomId : string) {
        console.debug(category, `Showing room "${roomId}" on chats page`)
        chatRoomList.selectRoom(roomId)
    }

    function useImageFromClipboard() {
        const chatRoom = SelectionState.selectedChatRoom
        if (chatRoom) {
            control.attachedData.uploadImageFromClipboard(chatRoom.id)
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
            leftMargin: Theme.d * 2
            rightMargin: Theme.d * 2
            bottomMargin: 6
        }

        BusyIndicator {
            running: connectingCard.visible
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: connectingCard.verticalCenter
                bottomMargin: Theme.d * 2
            }
        }

        Label {
            text: qsTr("Connecting...")
            font.pixelSize: 22
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.verticalCenter
                topMargin: Theme.d * 2
            }
        }
    }

    Card {
        id: verificationCard
        visible: false
        anchors {
            fill: parent
            leftMargin: Theme.d * 2
            rightMargin: Theme.d * 2
            bottomMargin: 6
        }

        DeviceVerification {
            anchors.fill: parent
            chatProvider: control.attachedData
        }
    }

    Card {
        id: sideBar
        width: Math.floor(control.width * 1 / 4)
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            leftMargin: Theme.d * 2
            rightMargin: Theme.d * 2
            bottomMargin: 6
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
                id: sortPopup
                y: sortCardHeadingButton.height
                contentHeight: sortCol.implicitHeight
                contentWidth: Math.max(
                                  sortButtonGroup.buttons.reduce((acc, val) => Math.max(acc, val.implicitWidth), 0),
                                  unreadOnTopCheckBox.implicitWidth + unreadOnTopCheckBox.anchors.leftMargin
                                                                    + unreadOnTopCheckBox.anchors.rightMargin)

                onClosed: () => sortPopup.destroy()

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
                rightMargin: Theme.d
            }

            onClicked: () => roomListMenuComponent.createObject(roomListCardHeadingButton).popup()
        }

        Component {
            id: roomListMenuComponent

            Menu {
                id: roomListMenu
                onClosed: () => roomListMenu.destroy()
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
                    showSectionHeader: false
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

                    onRoomSelected: roomId => chatRoomList.selectRoom(roomId)

                    readonly property Connections selectionStateConnections: Connections {
                        target: SelectionState
                        function onSelectedChatRoomChanged() {
                            const chatRoom = SelectionState.selectedChatRoom
                            if (chatRoom && (chatRoom.ownUserJoinState === IChatRoom.UserRoomState.Joined)) {
                                chatRoom.resetUnreadCount()
                            }
                        }
                    }

                    function selectRoom(roomId : string) {

                        SelectionState.selectedChatRoom = control.attachedData?.chatRoomByRoomId(roomId) ?? null

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
            leftMargin: Theme.d * 2
            rightMargin: Theme.d * 2
            bottomMargin: Theme.d / 2 + 2
        }

        Chat {
            id: chat
            anchors.fill: parent
            chatProvider: control.attachedData
            chatRoom: SelectionState.selectedChatRoom
        }
    }
}
