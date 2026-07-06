pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Controls.impl
import base

Popup {
    id: control
    transformOrigin: Item.Top
    topMargin: 12
    bottomMargin: 12
    verticalPadding: 8
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    enter: null  // Transitions seem to cause that the popup is not visible sometimes...
    exit: null
    visible: !!control.searchText.length

    LoggingCategory {
        id: category
        name: "gonnect.qml.SearchResultPopup"
        defaultLogLevel: LoggingCategory.Warning
    }

    Material.accent: Theme.accentColor
    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

    property string searchText

    signal primaryActionTriggered
    signal returnFocus

    readonly property int colWidth: flickableContainer.width / 3
    readonly property string immediateSearchPhrase: ViewHelper.preprocessSearchText(control.searchText)

    Timer {
        id: searchDebounceTimer
        interval: 200
        onTriggered: () => {
                         searchListModel.searchPhrase = control.immediateSearchPhrase
                         Qt.callLater(keyNavigator.keyDown)
                     }
    }

    onSearchTextChanged: () => {
                             if (control.immediateSearchPhrase === "") {
                                 searchDebounceTimer.stop()
                                 searchListModel.searchPhrase = ""
                             } else {
                                 searchDebounceTimer.start()
                             }
                         }

    function initialKeyDown() { keyNavigator.keyDown() }
    function initialKeyUp() { keyNavigator.keyUp() }

    function triggerPrimaryAction() {
        if (keyNavigator.selectedSubItem) {
            keyNavigator.selectedSubItem.triggerPrimaryAction()
            control.primaryActionTriggered()
        } else if (keyNavigator.selectedItem) {
            keyNavigator.selectedItem.triggerPrimaryAction()
            control.primaryActionTriggered()
        } else {
            console.error(category, 'cannot find selected item to trigger primary action on')
        }
    }

    function triggerSecondaryAction() {
        if (keyNavigator.selectedSubItem) {
            keyNavigator.selectedSubItem.triggerSecondaryAction()
        } else if (keyNavigator.selectedItem) {
            keyNavigator.selectedItem.triggerSecondaryAction()
        } else {
            console.error(category, 'cannot find selected item to trigger secondary action on')
        }
    }

    SearchListModel {
        id: searchListModel
    }

    contentItem: Item {
        id: popupContainer
        focus: true

        Keys.onLeftPressed: () => {
            if (LayoutMirroring.enabled) {
                keyNavigator.keyRight()
            } else {
                keyNavigator.keyLeft()
            }
        }
        Keys.onRightPressed: () => {
            if (LayoutMirroring.enabled) {
                keyNavigator.keyLeft()
            } else {
                keyNavigator.keyRight()
            }
        }
        Keys.onDownPressed: () => keyNavigator.keyDown()
        Keys.onUpPressed: () => keyNavigator.keyUp()
        Keys.onEnterPressed: () => control.triggerPrimaryAction()
        Keys.onReturnPressed: () => control.triggerPrimaryAction()
        Keys.onMenuPressed: () => control.triggerSecondaryAction()

        Item {
            id: filterBar
            width: 0.2 * control.width
            anchors {
                top: parent.top
                left: parent.left
                bottom: parent.bottom
                margins: 12
            }

            Accessible.role: Accessible.Column
            Accessible.name: qsTr("Search filter and identity selection")
            Accessible.description: qsTr("Select search filter to be applied, as well as the outgoing identity")

            SearchCategoryList {
                id: searchCategories
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
            }

            Label {
                id: identityLabel
                text: qsTr("Outgoing identity")
                font.weight: Font.Medium
                anchors {
                    left: identitySelector.left
                    right: identitySelector.right
                    bottom: identitySelector.top
                    bottomMargin: 5
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: identityLabel.text
            }

            IdentitySelector {
                id: identitySelector
                height: 30
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
            }
        }

        Rectangle {
            id: verticalSeparatorLine
            width: 1
            color: Theme.borderColor
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: filterBar.right
                leftMargin: 12
            }

            Accessible.ignored: true
        }

        Item {
            id: searchResultContainer
            anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
                left: verticalSeparatorLine.right
                margins: 12
            }

            Accessible.role: Accessible.Column
            Accessible.name: qsTr("Search results")
            Accessible.description: qsTr("All search results will be listed here in their respective categories")

            KeyNavigator {
                id: keyNavigator

                onVerticallyOutOfBounds: () => {
                    if (control.visible) {
                        control.returnFocus()
                    }
                }

                readonly property Connections resetConnection: Connections {
                    target: control
                    function onSearchTextChanged() { keyNavigator.reset() }
                }

                onSelectedItemChanged: () => {
                    if (!keyNavigator.selectedInternally) {
                        return
                    }

                    // Scroll selected item into view
                    const item = keyNavigator.selectedItem
                    if (item) {
                        const p = flickableContainer.mapFromItem(item.parent, item.x, item.y, item.width, item.height)
                        if (p.y + p.height - flickable.contentY > flickable.height || p.y < flickable.contentY) {
                            flickable.contentY = p.y
                        }
                    }
                }
            }

            Flickable {
                id: flickable
                clip: true
                contentHeight: flickableContainer.implicitHeight
                anchors {
                    top: parent.top
                    // bottom: showAllContactsButton.top
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                    topMargin: 12
                    bottomMargin: 12
                }

                ScrollBar.vertical: ScrollBar { width: 5 }

                Column {
                    id: flickableContainer
                    spacing: 24
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: 12
                    }

                    SearchResultCategory {
                        id: directDialList
                        headerText: qsTr('Direct dial')
                        visible: callDirectItem.shallBeVisible || roomDirectItem.shallBeVisible
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Component.onCompleted: () => keyNavigator.addContainer(directDialList, 0)

                        SearchResultItem {
                            id: callDirectItem
                            mainText: qsTr('Call "%1"').arg(control.immediateSearchPhrase)
                            width: control.colWidth
                            visible: callDirectItem.shallBeVisible
                            highlighted: keyNavigator.selectedItem === callDirectItem
                            mainRowLeftInsetLoader.sourceComponent: IconLabel {
                                color: Theme.primaryTextColor
                                icon {
                                    color: Theme.primaryTextColor
                                    source: Icons.callStart
                                }
                            }

                            readonly property bool shallBeVisible: ViewHelper.isPhoneNumber(control.immediateSearchPhrase)

                            onManuallyHovered: () => {
                                keyNavigator.setExternallySelected(callDirectItem)
                            }
                            onTriggerPrimaryAction: () => {
                                SIPCallManager.call("account0", control.immediateSearchPhrase, "", identitySelector.currentValue)
                                control.primaryActionTriggered()
                            }
                        }

                        SearchResultItem {
                            id: roomDirectItem
                            mainText: qsTr('Open room "%1"').arg(control.immediateSearchPhrase)
                            secondaryText: qsTr('Jitsi Meet')
                            width: control.colWidth
                            visible: roomDirectItem.shallBeVisible
                            highlighted: keyNavigator.selectedItem === roomDirectItem
                            mainRowLeftInsetLoader.sourceComponent: IconLabel {
                                color: Theme.primaryTextColor
                                icon {
                                    color: Theme.primaryTextColor
                                    source: Icons.userGroupNew
                                }
                            }

                            readonly property bool shallBeVisible: ViewHelper.isJitsiAvailable
                                                                   && !ViewHelper.isActiveVideoCall
                                                                   && ViewHelper.isValidJitsiRoomName(control.immediateSearchPhrase)

                            onManuallyHovered: () => {
                                keyNavigator.setExternallySelected(roomDirectItem)
                            }
                            onTriggerPrimaryAction: () => {
                                ViewHelper.requestMeeting(control.immediateSearchPhrase)
                                control.primaryActionTriggered()
                            }
                        }

                        SearchResultItem {
                            id: createChatRoomItem
                            mainText: qsTr('Create chat room "%1"').arg(control.immediateSearchPhrase)
                            width: control.colWidth
                            visible: ChatConnectorManager.isChatAvailable
                            canBeHighlighted: false
                            mainRowLeftInsetLoader.sourceComponent: IconLabel {
                                color: Theme.primaryTextColor
                                icon {
                                    color: Theme.primaryTextColor
                                    source: Icons.userGroupNew
                                }
                            }

                            Repeater {
                                id: phoneNumberRepeater
                                model: ChatConnectorManager.chatConnectors
                                delegate: SearchResultNumberItem {
                                    id: chatProviderDelg
                                    enabled: chatProviderDelg.modelData?.isConnected ?? false
                                    isChat: true
                                    isFavorable: false
                                    highlighted: keyNavigator.selectedSubItem === chatProviderDelg
                                    //: Search submenu item under "Create chatroom xyz"; %1 will be replaced with chat provider's display name
                                    number: qsTr("in %1").arg(chatProviderDelg.modelData?.displayName ?? "")
                                    anchors {
                                        left: parent?.left
                                        right: parent?.right
                                    }

                                    required property IChatProvider modelData

                                    onManuallyHovered: () => {
                                        keyNavigator.setExternallySelected(createChatRoomItem, chatProviderDelg)
                                    }
                                    onTriggerPrimaryAction: () => {
                                        ViewHelper.showCreateRoomDialog(chatProviderDelg.modelData,
                                                                        [],
                                                                        control.immediateSearchPhrase)
                                        control.primaryActionTriggered()
                                    }
                                }
                            }
                        }
                    }

                    SearchResultCategory {
                        id: chatRoomResultList
                        headerText: qsTr('Chat rooms')
                        visible: (searchCategories.selectedCategories & SearchCategoryList.Category.RoomsAndTeams)
                                 && chatRoomSearchRepeater.count > 0
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Component.onCompleted: () => keyNavigator.addContainer(chatRoomResultList, 1)

                        Repeater {
                            id: chatRoomSearchRepeater
                            model: AllChatProvidersRoomSearchProxyModel {
                                filterText: searchListModel.searchPhrase

                                AllChatProvidersRoomProxyModel {}
                            }
                            delegate: SearchResultItem {
                                id: chatRoomDelg
                                width: control.colWidth
                                mainText: chatRoomDelg.name
                                secondaryText: chatRoomDelg.chatProvider?.displayName ?? qsTr("Chat")
                                highlighted: keyNavigator.selectedItem === chatRoomDelg
                                mainRowLeftInsetLoader.sourceComponent: IconLabel {
                                    color: Theme.primaryTextColor
                                    icon {
                                        color: Theme.primaryTextColor
                                        source: Icons.dialogMessages
                                    }
                                }

                                required property string roomId
                                required property string name
                                required property IChatProvider chatProvider

                                onManuallyHovered: () => {
                                    keyNavigator.setExternallySelected(chatRoomDelg)
                                }
                                onTriggerPrimaryAction: () => {
                                    ViewHelper.showChatRoom(chatRoomDelg.chatProvider, chatRoomDelg.roomId)
                                    control.primaryActionTriggered()
                                }
                            }
                        }
                    }

                    SearchResultCategory {
                        id: historyResultList
                        headerText: qsTr('History')
                        visible: (searchCategories.selectedCategories & SearchCategoryList.Category.History)
                                 && historySearchRepeater.count > 0
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Component.onCompleted: () => keyNavigator.addContainer(historyResultList, 2)

                        Repeater {
                            id: historySearchRepeater
                            model: HistoryContactSearchProxyModel {
                                showJitsi: ViewHelper.isJitsiAvailable

                                HistoryContactSearchModel {
                                    searchText: searchListModel.searchPhrase
                                }
                            }
                            delegate: SearchResultItem {
                                id: historyDelg
                                width: control.colWidth
                                mainText: historyDelg.displayName || historyDelg.url
                                secondaryText: historyDelg.displayName ? historyDelg.url : ""
                                highlighted: keyNavigator.selectedItem === historyDelg
                                enabled: historyDelg.isPhoneNumber || ViewHelper.isJitsiAvailable
                                mainRowLeftInsetLoader.sourceComponent: IconLabel {
                                    color: Theme.primaryTextColor
                                    icon {
                                        color: Theme.primaryTextColor
                                        source: historyDelg.isPhoneNumber ? Icons.callStart : Icons.userGroupNew
                                    }
                                }

                                required property bool isPhoneNumber
                                required property bool isFavorite
                                required property bool isAnonymous
                                required property bool isBlocked
                                required property bool isSipSubscriptable
                                required property string displayName
                                required property string url

                                onManuallyHovered: () => {
                                    keyNavigator.setExternallySelected(historyDelg)
                                }
                                onTriggerPrimaryAction: () => {
                                    if (historyDelg.isPhoneNumber) {
                                        SIPCallManager.call("account0", historyDelg.url, "", identitySelector.currentValue);
                                    } else if (!ViewHelper.isActiveVideoCall) {
                                        ViewHelper.requestMeeting(historyDelg.url)
                                    }
                                    control.primaryActionTriggered()
                                }
                                onTriggerSecondaryAction: () => {
                                    if (historyDelg.isPhoneNumber) {
                                        historyDelg.historyContextMenuComponent.createObject(historyDelg).popup()
                                    } else {
                                        historyDelg.jitsiHistoryListContextMenuComponent.createObject(historyDelg).popup()
                                    }
                                }

                                readonly property Component historyContextMenuComponent: Component {
                                    HistoryListContextMenu {
                                        id: historyContextMenu
                                        phoneNumber: historyDelg.url
                                        isFavorite: historyDelg.isFavorite
                                        isAnonymous: historyDelg.isAnonymous
                                        isReady: historyContextMenu.buddyStatus === SIPBuddyState.READY
                                        isSipSubscriptable: historyDelg.isSipSubscriptable
                                        isBlocked: historyDelg.isBlocked
                                        width: 230

                                        property int buddyStatus: SIPBuddyState.UNKNOWN

                                        onCallClicked: () => {
                                            if (historyDelg.isPhoneNumber) {
                                                SIPCallManager.call("account0", historyDelg.url, "", identitySelector.currentValue);
                                            } else if (!ViewHelper.isActiveVideoCall) {
                                                ViewHelper.requestMeeting(historyDelg.url)
                                            }
                                            control.primaryActionTriggered()
                                        }
                                        Component.onCompleted: () => {
                                            historyContextMenu.updateBuddyStatus()
                                        }

                                        function subscribeBuddyStatus() {
                                            const buddy = SIPManager.getBuddy(historyDelg.url)
                                            if (buddy !== null) {
                                                buddy.subscribeToBuddyStatus()
                                            }
                                        }

                                        function updateBuddyStatus() {
                                            historyContextMenu.buddyStatus = SIPManager.buddyStatus(historyDelg.url)
                                        }

                                        onNotifyWhenAvailableClicked: () => historyDelg.subscribeBuddyStatus()
                                        onBlockTemporarilyClicked: () => SIPCallManager.toggleTemporaryBlock("", historyDelg.remotePhoneNumber)
                                    }
                                }

                                readonly property Component jitsiHistoryListContextMenuComponent: Component {
                                    JitsiHistoryListContextMenu {
                                        id: jitsiHistoryListContextMenu
                                        isFavorite: historyDelg.isFavorite
                                        roomName: historyDelg.url
                                        width: 230
                                        onCallClicked: () => {
                                            if (!ViewHelper.isActiveVideoCall) {
                                                ViewHelper.requestMeeting(historyDelg.url)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Repeater {
                        id: contactReapeater

                        readonly property ContactSourceInfoModel contactSourceInfoModel: ContactSourceInfoModel {}

                        model: (searchCategories.selectedCategories & SearchCategoryList.Category.Contacts)
                               ? contactReapeater.contactSourceInfoModel
                               : null

                        delegate: SearchResultCategory {
                            id: contactsSourceDelegate
                            visible: searchResultRepeater.count > 0
                            headerText: contactsSourceDelegate.displayName || qsTr('Contacts')
                            anchors {
                                left: parent?.left
                                right: parent?.right
                            }

                            required property int index
                            required property string displayName

                            Component.onCompleted: () => keyNavigator.addContainer(contactsSourceDelegate, contactsSourceDelegate.index + 2)

                            Repeater {
                                id: searchResultRepeater
                                model: SearchListProxyModel {
                                    sourceDisplayName: contactsSourceDelegate.displayName
                                    sourceModel: searchListModel
                                }

                                delegate: SearchResultItem {
                                    id: contactDelg
                                    width: control.colWidth
                                    mainText: contactDelg.name
                                    secondaryText: contactDelg.company
                                    highlighted: keyNavigator.selectedItem === contactDelg
                                    canBeHighlighted: contactDelg.numbersCount === 0

                                    required property int index
                                    required property string sourceDisplayName
                                    required property string id
                                    required property string name
                                    required property string company
                                    required property string avatarPath
                                    required property string subscriptableNumber
                                    required property bool hasAvatar
                                    required property var numbers
                                    required property int numbersCount
                                    required property int numbersIndexOffset
                                    required property var chatSources

                                    readonly property bool isDummyContact: contactDelg.name === ""

                                    property int buddyStatus: SIPBuddyState.UNKNOWN

                                    function updateBuddyStatus() {
                                        contactDelg.buddyStatus = contactDelg.subscriptableNumber !== 0
                                                ? SIPManager.buddyStatus(contactDelg.subscriptableNumber)
                                                : SIPBuddyState.UNKNOWN
                                    }

                                    onManuallyHovered: () => {
                                        keyNavigator.setExternallySelected(contactDelg)
                                    }

                                    Component.onCompleted: () => contactDelg.updateBuddyStatus()

                                    readonly property Connections sipManagerConnections: Connections {
                                        target: SIPManager
                                        enabled: contactDelg.subscriptableNumber !== ""
                                        function onBuddyStateChanged(url : string, status : int) {
                                            contactDelg.updateBuddyStatus()
                                        }
                                    }

                                    mainRowLeftInsetLoader.sourceComponent: Component {
                                        AvatarImage {
                                            id: avatarImage
                                            initials: ViewHelper.initials(contactDelg.name)
                                            source: contactDelg.hasAvatar ? ("file://" + contactDelg.avatarPath) : ""
                                            showPresenceStatus: contactDelg.subscriptableNumber !== ""
                                            presenceStatus: contactDelg.buddyStatus
                                            indicatorComponent: Component { BuddyStatusIndicator {} }
                                        }
                                    }

                                    Repeater {
                                        id: phoneNumberRepeater
                                        model: contactDelg.numbers
                                        delegate: SearchResultNumberItem {
                                            id: numberDelg
                                            type: numberDelg.modelData.type
                                            highlighted: keyNavigator.selectedSubItem === numberDelg
                                            number: numberDelg.modelData.number
                                            isSipStatusSubscriptable: numberDelg.modelData.isSipStatusSubscriptable
                                            isFavorite: numberDelg.modelData.isFavorite
                                            contactId: contactDelg.id
                                            anchors {
                                                left: parent?.left
                                                right: parent?.right
                                            }

                                            required property var modelData

                                            onManuallyHovered: () => {
                                                keyNavigator.setExternallySelected(contactDelg, numberDelg)
                                            }
                                            onTriggerPrimaryAction: () => {
                                                SIPCallManager.call("account0", numberDelg.number, contactDelg.id, identitySelector.currentValue)
                                                control.primaryActionTriggered()
                                            }
                                            onTriggerSecondaryAction: () => {
                                                numberDelg.numberContextMenuComponent.createObject(numberDelg).popup()
                                            }

                                            function subscribeBuddyStatus() {
                                                const buddy = SIPManager.getBuddy(numberDelg.number)
                                                if (buddy !== null) {
                                                    buddy.subscribeToBuddyStatus()
                                                }
                                            }

                                            readonly property Component numberContextMenuComponent: Component {
                                                HistoryListContextMenu {
                                                    id: numberContextMenu
                                                    phoneNumber: numberDelg.number
                                                    isFavorite: numberDelg.isFavorite
                                                    isSipSubscriptable: numberDelg.isSipStatusSubscriptable
                                                    favoriteAvailable: !contactDelg.isDummyContact
                                                    isReady: contactDelg.buddyStatus === SIPBuddyState.READY

                                                    onCallClicked: () => {
                                                        SIPCallManager.call("account0", numberDelg.number.url, "", identitySelector.currentValue);
                                                        control.primaryActionTriggered()
                                                    }
                                                    onNotifyWhenAvailableClicked: () => numberDelg.subscribeBuddyStatus()
                                                }
                                            }
                                        }
                                    }

                                    Repeater {
                                        id: chatSourceRepeater
                                        model: contactDelg.chatSources
                                        delegate: SearchResultNumberItem {
                                            id: chatSourceDelg
                                            highlighted: keyNavigator.selectedSubItem === chatSourceDelg
                                            isChat: true
                                            isSipStatusSubscriptable: false
                                            isFavorite: false
                                            type: Contact.NumberType.Mobile
                                            contactId: chatSourceDelg.modelData.id
                                            number: chatSourceDelg.modelData.providerDisplayName
                                            anchors {
                                                left: parent?.left
                                                right: parent?.right
                                            }

                                            required property var modelData

                                            onManuallyHovered: () => {
                                                keyNavigator.setExternallySelected(contactDelg, chatSourceDelg)
                                            }
                                            onTriggerPrimaryAction: () => {
                                                const chatRoomId = chatSourceDelg.modelData.provider.chatRoomIdForUser(chatSourceDelg.modelData.id)
                                                if (chatRoomId) {
                                                    console.log(`Request showing chat room ${chatRoomId} of ${chatSourceDelg.modelData.provider?.displayName}`)
                                                    ViewHelper.showChatRoom(chatSourceDelg.modelData.provider, chatRoomId)
                                                }

                                                control.primaryActionTriggered()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Keep for later when we know what to show when clicking on this

            // Label {
            //     id: showAllContactsButton
            //     color: showAllContactsButtonHoverHandler.hovered ? Theme.primaryTextColor : Theme.secondaryTextColor
            //     text: qsTr('Show all phone contacts')
            //     anchors {
            //         horizontalCenter: parent.horizontalCenter
            //         bottom: parent.bottom
            //         bottomMargin: 10
            //     }

            //     Behavior on color { ColorAnimation {} }

            //     HoverHandler {
            //         id: showAllContactsButtonHoverHandler
            //     }

            //     TapHandler {
            //         onTapped: () => console.log(category, 'TODO')
            //     }
            // }
        }
    }

    background: Rectangle {
        radius: 4
        color: parent.Material.dialogColor

        layer.enabled: control.enabled
        layer.effect: RoundedElevationEffect {
            elevation: 4
            roundedScale: Material.ExtraSmallScale
        }
    }
}

