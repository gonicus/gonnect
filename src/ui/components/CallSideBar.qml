pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitWidth: control.extended ? control.optimalExtendedWidth : 70

    /// Whether the side bar is extended or collapsed
    readonly property bool extended: control.selectedSideBarMode !== CallSideBar.None

    /// Like the implicitWidth, but for extended state only
    readonly property int optimalExtendedWidth: {
        let w = 0

        if (chatButton.visible) {
            w += chatButton.width
        }
        if (personCountButton.visible) {
            w += personCountButton.width
        }
        if (additionalInfoButton.visible) {
            w += additionalInfoButton.width
        }

        return w
    }

    /// What the side bar shall show (value of the enum SideBarMode)
    property int selectedSideBarMode: CallSideBar.None

    /// Whether the chat is available at all (default: false)
    property alias chatAvailable: chatButton.visible

    /// Whether the list of persons/users is available at all (default: true)
    property alias personsAvailable: personCountButton.visible

    property IConferenceConnector conferenceConnector

    property AggregatedDirectRoomsOfContact roomsAggregator: null

    function useImageFromClipboard() {
        if (chatSideBar.visible && chatSideBar.chatProvider && chatSideBar.chatRoom) {
            chatSideBar.chatProvider.uploadImageFromClipboard(chatSideBar.chatRoom.id)
        }
    }

    onChatAvailableChanged: {
        if (!control.chatAvailable && control.selectedSideBarMode === CallSideBar.Chat) {
            control.selectedSideBarMode === CallSideBar.None
        }
    }

    onSelectedCallItemChanged: () => {
        if (!control.selectedCallItem) {
            control.selectedSideBarMode = CallSideBar.None
        }
    }

    enum SideBarMode {
        None,
        Caller,
        Chat,
        AdditionalInfo,
        Users
    }

    readonly property alias selectedCallItem: callList.selectedItem

    QtObject {
        id: internal

        readonly property Connections callListConnections: Connections {
            target: callList
            function onCountChanged() { internal.updateAutoExtendCollapse() }
        }

        readonly property Connections userListConnections: Connections {
            target: userList
            function onCountChanged() { internal.updateAutoExtendCollapse() }
        }

        function updateAutoExtendCollapse() {
            const count = callList.count

            if (count > 1 && !control.extended) {
                control.selectedSideBarMode = userList.count
                                              ? CallSideBar.SideBarMode.Users
                                              : CallSideBar.SideBarMode.Caller
            } else if (count <= 1 && control.extended) {
                control.selectedSideBarMode = CallSideBar.SideBarMode.None
            }
        }
    }

    Item {
        id: sideBarModeState
        states: [
            State {
                when: control.selectedSideBarMode === CallSideBar.Caller
                PropertyChanges {
                    callList.visible: true
                    personCountButton.highlighted: true
                }
            },
            State {
                when: control.selectedSideBarMode === CallSideBar.Chat
                PropertyChanges {
                    chatSideBar.visible: true
                    chatButton.highlighted: true
                }
            },
            State {
                when: control.selectedSideBarMode === CallSideBar.AdditionalInfo
                PropertyChanges {
                    additionalInfo.visible: true
                    additionalInfoButton.highlighted: true
                }
            },
            State {
                when: control.selectedSideBarMode === CallSideBar.Users
                PropertyChanges {
                    userList.visible: true
                    personCountButton.highlighted: true
                }
            }
        ]
    }

    Item {
        id: extendedState
        states: [
            State {
                when: control.extended
                PropertyChanges {
                    control.implicitWidth: 300
                }
            },

            State {
                when: !control.extended
                AnchorChanges {
                    target: chatButton
                    anchors {
                        right: undefined
                        horizontalCenter: headerBar.horizontalCenter
                        top: headerBar.top
                    }
                }
                AnchorChanges {
                    target: personCountButton
                    anchors {
                        right: undefined
                        horizontalCenter: headerBar.horizontalCenter
                        top: control.chatAvailable ? chatButton.bottom : headerBar.top
                    }
                }
                AnchorChanges {
                    target: additionalInfoButton
                    anchors {
                        right: undefined
                        horizontalCenter: headerBar.horizontalCenter
                        top: personCountButton.visible
                             ? personCountButton.bottom
                             : (chatButton.visible
                                ? chatButton.bottom
                                : headerBar.top)
                    }
                }
            }
        ]
    }

    Item {
        id: headerBar
        implicitHeight: personCountButton.implicitHeight
        height: headerBar.implicitHeight
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        BarButton {
            id: chatButton
            anchors.right: personCountButton.visible
                           ? personCountButton.left
                           : (additionalInfoButton.visible
                              ? additionalInfoButton.left
                              : parent.right)
            anchors.rightMargin: 5
            iconPath: Icons.dialogMessages
            visible: !!(control.roomsAggregator?.chatRooms.length)
            text: qsTr("Chat")
            showIndicatorBadge: !chatSideBar.visible && chatSideBar.lastMessageCount < (chatSideBar.chatRoom?.notificationCount ?? 0)
            onClicked: () => {
                if (control.selectedSideBarMode === CallSideBar.Chat) {
                    control.selectedSideBarMode = CallSideBar.None
                } else {
                    control.selectedSideBarMode = CallSideBar.Chat
                }
            }
        }

        BarButton {
            id: personCountButton
            anchors.right: additionalInfoButton.visible ? additionalInfoButton.left : parent.right
            anchors.rightMargin: 5
            iconPath: Icons.avatarDefault
            iconText: callList.count + userList.count
            text: qsTr("Person(s)", "", callList.count + userList.count)
            onClicked: () => {
                if (control.selectedSideBarMode === CallSideBar.Caller || control.selectedSideBarMode === CallSideBar.Users) {
                    control.selectedSideBarMode = CallSideBar.None
                } else if (userList.count) {
                    control.selectedSideBarMode = CallSideBar.Users
                } else {
                    control.selectedSideBarMode = CallSideBar.Caller
                }
            }
        }

        BarButton {
            id: additionalInfoButton
            anchors.right: parent.right
            anchors.rightMargin: 5
            iconPath: Icons.helpAbout
            visible: control.selectedCallItem?.hasMetadata ?? false
            text: qsTr("Info")
            onClicked: () => {
                if (control.selectedSideBarMode === CallSideBar.AdditionalInfo) {
                    control.selectedSideBarMode = CallSideBar.None
                } else {
                    control.selectedSideBarMode = CallSideBar.AdditionalInfo
                }
            }
        }

        Rectangle {
            id: headerBarBorder
            height: 1
            visible: control.extended  // For some reason, that does not work when setting via states
            color: Theme.borderColor
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
        }
    }

    CallList {
        id: callList
        clip: true
        visible: false
        showHangupButton: SIPCallManager.isConferenceMode
        anchors {
            top: headerBar.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
    }

    UsersList {
        id: userList
        conferenceConnector: control.conferenceConnector
        clip: true
        visible: false
        anchors {
            top: headerBar.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
    }

    ChatSideBar {
        id: chatSideBar
        visible: false
        chatProvider: control.roomsAggregator?.providerOfRoom(chatSideBar.chatRoom) ?? null
        chatRoom: {
            const aggr = control.roomsAggregator
            if (aggr && aggr.bestMatchingChatRoom) {
                return aggr.bestMatchingChatRoom
            }
            if (control.conferenceConnector) {
                return control.conferenceConnector.chatRoom()
            }
            return null
        }
        anchors {
            top: headerBar.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        property int lastMessageCount: 0

        Component.onCompleted: () => chatSideBar.updateChatRoom()

        onVisibleChanged: () => {
            if (!chatSideBar.visible) {
                chatSideBar.lastMessageCount = chatSideBar.chatRoom?.notificationCount ?? 0
            }
        }

        Connections {
            target: control.conferenceConnector
            function onIsInConferenceChanged() {
                chatSideBar.lastMessageCount = 0
                chatSideBar.updateChatRoom()
            }
        }

        Connections {
            target: control
            function onRoomsAggregatorChanged() { chatSideBar.updateChatRoom() }
        }

        Connections {
            target: control.roomsAggregator
            function onBestMatchingChatRoomChanged() { chatSideBar.updateChatRoom() }
        }

        function updateChatRoom() {
            Qt.callLater(() => {
                const aggr = control.roomsAggregator
                if (aggr && aggr.bestMatchingChatRoom) {
                    chatSideBar.chatRoom = aggr.bestMatchingChatRoom
                    return
                }
                if (control.conferenceConnector) {
                    chatSideBar.chatRoom = control.conferenceConnector.chatRoom()
                    return
                }
                chatSideBar.chatRoom = null
            })
        }
    }

    AdditionalInfo {
        id: additionalInfo
        visible: false
        anchors {
            top: headerBar.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        callItem: control.selectedCallItem
    }
}
