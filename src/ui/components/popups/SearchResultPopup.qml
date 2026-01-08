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
            console.error('cannot find selected item to trigger primary action on')
        }
    }

    function triggerSecondaryAction() {
        if (keyNavigator.selectedSubItem) {
            keyNavigator.selectedSubItem.triggerSecondaryAction()
        } else if (keyNavigator.selectedItem) {
            keyNavigator.selectedItem.triggerSecondaryAction()
        } else {
            console.error('cannot find selected item to trigger secondary action on')
        }
    }

    SearchListModel {
        id: searchListModel
        searchPhrase: ViewHelper.preprocessSearchText(control.searchText)
    }

    contentItem: Item {
        id: popupContainer
        focus: true

        Keys.onLeftPressed: () => keyNavigator.keyLeft()
        Keys.onRightPressed: () => keyNavigator.keyRight()
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

            SearchCategoryList {
                id: searchCategories
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
            }

            Label {
                text: qsTr("Outgoing identity")
                font.weight: Font.Medium
                anchors {
                    left: identitySelector.left
                    right: identitySelector.right
                    bottom: identitySelector.top
                    bottomMargin: 5
                }
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

            KeyNavigator {
                id: keyNavigator

                onVerticallyOutOfBounds: () => control.returnFocus()

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
                            mainText: qsTr('Call "%1"').arg(searchListModel.searchPhrase)
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

                            readonly property bool shallBeVisible: ViewHelper.isPhoneNumber(searchListModel.searchPhrase)

                            onManuallyHovered: () => {
                                keyNavigator.setExternallySelected(callDirectItem)
                            }
                            onTriggerPrimaryAction: () => {
                                SIPCallManager.call("account0", searchListModel.searchPhrase, "", identitySelector.currentValue)
                                control.primaryActionTriggered()
                            }
                        }

                        SearchResultItem {
                            id: roomDirectItem
                            mainText: qsTr('Open room "%1"').arg(searchListModel.searchPhrase)
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
                                                                   && ViewHelper.isValidJitsiRoomName(searchListModel.searchPhrase)

                            onManuallyHovered: () => {
                                keyNavigator.setExternallySelected(roomDirectItem)
                            }
                            onTriggerPrimaryAction: () => {
                                ViewHelper.requestMeeting(searchListModel.searchPhrase)
                                control.primaryActionTriggered()
                            }
                        }
                    }

                    SearchResultCategory {
                        id: historyResultList
                        headerText: qsTr('History')
                        visible: historySearchRepeater.count > 0
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Component.onCompleted: () => keyNavigator.addContainer(historyResultList, 1)

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
                                enabled: ViewHelper.isJitsiAvailable || !historyDelg.isPhoneNumber
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
                        model: ContactSourceInfoModel {}
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
                                            showBuddyStatus: contactDelg.subscriptableNumber !== ""
                                            buddyStatus: contactDelg.buddyStatus
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
            //         onTapped: () => console.log('TODO')
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

