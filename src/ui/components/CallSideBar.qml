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
            w += chatButton.width + Theme.d
        }
        for (let i = 0; i < roomsRepeater.count; ++i) {
            const item = roomsRepeater.itemAt(i)
            if (item && item.visible) {
                w += item.width + Theme.d
            }
        }
        if (transcriptionButton.visible) {
            w += transcriptionButton.width + Theme.d
        }
        if (personCountButton.visible) {
            w += personCountButton.width + Theme.d
        }
        if (additionalInfoButton.visible) {
            w += additionalInfoButton.width + Theme.d
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

    property bool conferenceMode: false

    /// Whether the conference chat is currently used (as opposed to a direct chat, for instance)
    property bool conferenceChatInUse: false

    property AggregatedDirectRoomsOfContact roomsAggregator: null

    // null: automatic selection, -1 is conference chat, else { provider, roomId }
    property var manualChatSource: null

    property int linkedRoomsCount: 0

    ConferenceLinkedRoomsProxyModel {
        id: conferenceRoomsModel
        conferenceUrl: (control.conferenceMode && (control.conferenceConnector?.isInConference ?? false))
                       ? (control.conferenceConnector?.conferenceUrl.toString() ?? "")
                       : ""

        AllChatProvidersRoomProxyModel {}
    }

    function useImageFromClipboard() {
        if (chatSideBar.visible && chatSideBar.chatProvider && chatSideBar.chatRoom) {
            chatSideBar.chatProvider.uploadImageFromClipboard(chatSideBar.chatRoom.id)
        }
    }

    onChatAvailableChanged: {
        if (!control.chatAvailable && control.selectedSideBarMode === CallSideBar.Chat) {
            control.selectedSideBarMode = CallSideBar.None
        }
    }

    onSelectedCallItemChanged: () => {
        if (!control.selectedCallItem && !control.conferenceMode) {
            control.selectedSideBarMode = CallSideBar.None
        }
    }

    enum SideBarMode {
        None,
        Caller,
        Chat,
        AdditionalInfo,
        Users,
        Transcription
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
            },
            State {
                when: control.selectedSideBarMode === CallSideBar.Transcription
                PropertyChanges {
                    transcriptionSideBar.visible: true
                    transcriptionButton.highlighted: true
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

        // No Row/Column/ParentChange here on purpose: reparenting buttons between a Row and a
        // Column turned out fragile (binding loops on "extended", stale x/y from the previously
        // active positioner, and clicks not reliably registering while a reparent was in
        // flight). Buttons stay permanent children of headerBar; their x/y are computed and
        // assigned explicitly instead.
        readonly property int buttonSpacing: Theme.d

        readonly property bool isLinkedRoomShown: {
            for (let i = 0; i < roomsRepeater.count; ++i) {
                const item = roomsRepeater.itemAt(i)
                if (item && item.chatProvider === chatSideBar.chatProvider && item.roomId === chatSideBar.chatRoom?.id) {
                    return true
                }
            }
            return false
        }

        function relayoutButtons() {
            const items = []
            if (chatButton.visible) {
                items.push(chatButton)
            }
            for (let i = 0; i < roomsRepeater.count; ++i) {
                const item = roomsRepeater.itemAt(i)
                if (item && item.visible) {
                    items.push(item)
                }
            }
            if (transcriptionButton.visible) {
                items.push(transcriptionButton)
            }
            if (personCountButton.visible) {
                items.push(personCountButton)
            }
            if (additionalInfoButton.visible) {
                items.push(additionalInfoButton)
            }

            if (control.extended) {
                // Right-aligned horizontal row
                let xPos = headerBar.width - headerBar.buttonSpacing
                for (let i = items.length - 1; i >= 0; --i) {
                    const it = items[i]
                    xPos -= it.width
                    it.x = xPos
                    it.y = (headerBar.implicitHeight - it.height) / 2
                    xPos -= headerBar.buttonSpacing
                }
            } else {
                // Centered vertical column
                let yPos = 0
                for (const it of items) {
                    it.x = (headerBar.width - it.width) / 2
                    it.y = yPos
                    yPos += it.height + headerBar.buttonSpacing
                }
            }
        }

        // Single hashed trigger so every relevant change (extended, visibility of the static
        // buttons, number of room tabs) causes exactly one relayout call.
        readonly property int layoutTrigger: {
            let t = control.extended ? 1 : 0
            t = t * 2 + (chatButton.visible ? 1 : 0)
            t = t * 2 + (transcriptionButton.visible ? 1 : 0)
            t = t * 2 + (personCountButton.visible ? 1 : 0)
            t = t * 2 + (additionalInfoButton.visible ? 1 : 0)
            t = t * 1000 + roomsRepeater.count
            return t
        }
        onLayoutTriggerChanged: headerBar.relayoutButtons()
        onWidthChanged: headerBar.relayoutButtons()
        Component.onCompleted: headerBar.relayoutButtons()

        Connections {
            target: roomsRepeater
            function onItemAdded() { headerBar.relayoutButtons() }
            function onItemRemoved() { headerBar.relayoutButtons() }
        }

        BarButton {
            id: chatButton
            iconPath: Icons.dialogMessages
            visible: !!(control.roomsAggregator?.chatRooms.length)
            text: qsTr("Chat")
            highlighted: control.selectedSideBarMode === CallSideBar.Chat && !headerBar.isLinkedRoomShown
            showIndicatorBadge: (control.conferenceConnector?.chatRoom()?.notificationCount ?? 0) > 0
            labelElide: control.extended ? Text.ElideNone : Text.ElideRight

            readonly property bool isConferenceChatSource: control.conferenceConnector?.hasCapability(IConferenceConnector.Capability.ChatInCall) ?? false

            onClicked: () => {
                if (control.selectedSideBarMode === CallSideBar.Chat && !headerBar.isLinkedRoomShown) {
                    control.selectedSideBarMode = CallSideBar.None
                } else {
                    control.selectedSideBarMode = CallSideBar.Chat
                    control.manualChatSource = chatButton.isConferenceChatSource ? -1 : null
                    chatSideBar.updateChatRoomNow()
                }
            }
        }

        Repeater {
            id: roomsRepeater
            model: conferenceRoomsModel
            delegate: BarButton {
                id: roomDelg

                required property string roomId
                required property string name
                required property int unreadCount
                required property IChatProvider chatProvider

                iconPath: Icons.dialogMessages
                text: roomDelg.name
                highlighted: control.selectedSideBarMode === CallSideBar.Chat
                             && chatSideBar.chatProvider === roomDelg.chatProvider
                             && chatSideBar.chatRoom?.id === roomDelg.roomId
                showIndicatorBadge: roomDelg.unreadCount > 0
                labelElide: control.extended ? Text.ElideNone : Text.ElideRight
                onClicked: () => {
                               const isShown = chatSideBar.chatProvider === roomDelg.chatProvider && chatSideBar.chatRoom?.id === roomDelg.roomId
                               if (control.selectedSideBarMode === CallSideBar.Chat && isShown) {
                                   control.selectedSideBarMode = CallSideBar.None
                               } else {
                                   control.selectedSideBarMode = CallSideBar.Chat
                                   control.manualChatSource = { provider : roomDelg.chatProvider, roomId : roomDelg.roomId }
                                   chatSideBar.updateChatRoomNow()
                               }
                           }
            }
        }

        BarButton {
            id: transcriptionButton
            iconPath: Icons.addSubtitle
            visible: control.conferenceConnector?.isTranscriptionActive ?? false
            text: qsTr("Transcription")
            highlighted: control.selectedSideBarMode === CallSideBar.Transcription
            labelElide: control.extended ? Text.ElideNone : Text.ElideRight
            onClicked: () => {
                           if (control.selectedSideBarMode === CallSideBar.Transcription) {
                               control.selectedSideBarMode = CallSideBar.None
                           } else {
                               control.selectedSideBarMode = CallSideBar.Transcription
                           }
                       }
        }

        BarButton {
            id: personCountButton
            iconPath: Icons.avatarDefault
            iconText: callList.count + userList.count
            text: qsTr("Person(s)", "", callList.count + userList.count)
            labelElide: control.extended ? Text.ElideNone : Text.ElideRight
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
            iconPath: Icons.helpAbout
            visible: control.selectedCallItem?.hasMetadata ?? false
            text: qsTr("Info")
            labelElide: control.extended ? Text.ElideNone : Text.ElideRight
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
        anchors {
            top: headerBar.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        Component.onCompleted: () => chatSideBar.updateChatRoom()

        onVisibleChanged: () => {
            if (chatSideBar.visible) {
                chatSideBar.giveFocus()
            }
        }

        Connections {
            target: control.conferenceConnector
            function onIsInConferenceChanged() {
                if (!control.conferenceConnector.isInConference) {
                    control.conferenceChatInUse = false
                    control.manualChatSource = null
                }
                chatSideBar.updateChatRoom()
            }

            function onNumberOfUsersChanged() {
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

        Connections {
            target: conferenceRoomsModel
            function onRowsInserted() {
                control.linkedRoomsCount = conferenceRoomsModel.rowCount()
                chatSideBar.updateChatRoom()
            }
            function onRowsRemoved() {
                control.linkedRoomsCount = conferenceRoomsModel.rowCount()
                chatSideBar.updateChatRoom()
            }
            function onModelReset() {
                control.linkedRoomsCount = conferenceRoomsModel.rowCount()
                chatSideBar.updateChatRoom()
            }
        }


        // Synchronous core - call this directly from user-initiated actions (e.g. clicking a
        // tab) where the result is already known and must apply immediately, so chatSideBar
        // doesn't briefly become visible with its previous (stale) chatRoom first. Reactive/
        // automatic triggers (model or connector signals) should go through updateChatRoom()
        // instead, which defers and coalesces rapid bursts of such signals.
        function updateChatRoomNow() {
            const manualSource = control.manualChatSource

            if (manualSource !== null) {
                if (manualSource !== -1 && !manualSource.provider?.chatRoomByRoomId(manualSource.roomId)) {
                    control.manualChatSource = null
                } else if (manualSource === -1) {
                    chatSideBar.chatProvider = null
                    chatSideBar.chatRoom = control.conferenceConnector?.chatRoom() ?? null
                } else {
                    chatSideBar.chatProvider = manualSource.provider
                    chatSideBar.chatRoom = manualSource.provider.chatRoomByRoomId(manualSource.roomId)
                }

                return
            }

            // Override roomsAggregator chat room if more participants joined
            if (control.conferenceConnector && (control.conferenceChatInUse
                                                || (control.conferenceConnector && control.conferenceConnector.numberOfUsers > 2))) {
                control.conferenceChatInUse = true
                chatSideBar.chatRoom = control.conferenceConnector.chatRoom()
                chatSideBar.chatProvider = null
                return
            }

            if (conferenceRoomsModel.rowCount() > 0) {
                chatSideBar.chatRoom = conferenceRoomsModel.chatRoomAt(0)
                chatSideBar.chatProvider = conferenceRoomsModel.chatProviderAt(0)
                return
            }

            const aggr = control.roomsAggregator
            if (aggr && aggr.bestMatchingChatRoom) {
                chatSideBar.chatRoom = aggr.bestMatchingChatRoom
                chatSideBar.chatProvider = aggr.providerOfRoom(chatSideBar.chatRoom)
                return
            }
            if (control.conferenceConnector) {
                chatSideBar.chatRoom = control.conferenceConnector.chatRoom()
                chatSideBar.chatProvider = null
                return
            }
            chatSideBar.chatRoom = null
            chatSideBar.chatProvider = null
        }

        function updateChatRoom() {
            Qt.callLater(chatSideBar.updateChatRoomNow)
        }
    }

    TranscriptionSideBar {
        id: transcriptionSideBar
        conferenceConnector: control.conferenceConnector
        visible: false
        anchors {
            top: headerBar.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
    }

    Connections {
        target: control.conferenceConnector
        function onIsTranscriptionActiveChanged() {
            if (control.conferenceConnector.isTranscriptionActive) {
                control.selectedSideBarMode = CallSideBar.Transcription
            } else if (control.selectedSideBarMode === CallSideBar.Transcription) {
                control.selectedSideBarMode = CallSideBar.None
            }
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
