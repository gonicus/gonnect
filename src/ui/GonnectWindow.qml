pragma ComponentBehavior: Bound

import QtQuick
import QtCore
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "gonnectWindow"
    width: 910
    height: 554
    minimumWidth: 910
    minimumHeight: 600
    title: "GOnnect"
    resizable: true

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    readonly property LoggingCategory lc: LoggingCategory {
        id: category
        name: "gonnect.qml.GonnectWindow"
        defaultLogLevel: LoggingCategory.Warning
    }

    windowHeaderComponent: Component {
        CustomWindowHeader {
            mainBarWidth: mainTabBar.width
            mainBarColor: mainTabBar.backgroundColor

            showSearch: !SM.uiEditMode

            tabRoot: mainTabBar
            pageRoot: pageStack
        }
    }

    onActiveChanged: () => {
        if (control.active) {
            SIPCallManager.resetMissedCalls()
        }
    }

    function ensureVisible() {
        if (!control.visibility) {
            control.show()
        } else if (!control.active) {
            control.requestActivate()
        }
    }

    readonly property Settings windowSettings: Settings {
        id: settings
        location: ViewHelper.userConfigPath
        category: "generic"

        property alias gonnectWindowWidth: control.width
        property alias gonnectWindowHeight: control.height
    }

    enum PageType {
        Base,
        Call,
        Chats,
        Conference,
        Settings
    }

    // INFO: Static page ID's
    property string homePageId: "page_home"
    property string callPageId: "page_call"
    property string chatsPageId: "page_chats"
    property string conferencePageId: "page_conference"
    property string settingsPageId: "page_settings"

    property var previousPage

    readonly property CallsModel globalCallsModel: CallsModel {
        id: callsModel
    }

    readonly property Connections globalStateConnections: Connections {
        target: GlobalCallState

        function onCallStarted(isConference : bool) {
            if (isConference) {
                control.updateTabSelection(control.conferencePageId,
                                           GonnectWindow.PageType.Conference)
            } else {
                control.updateTabSelection(control.callPageId,
                                           GonnectWindow.PageType.Call)
            }
        }

        function onCallEnded(isConference : bool) {
            const count = GlobalCallState.activeCallsCount
            const isOnCallPage = mainTabBar.selectedPageType === GonnectWindow.PageType.Call
            const isOnConferencePage = mainTabBar.selectedPageType === GonnectWindow.PageType.Conference

            if (count && isOnCallPage && ViewHelper.isActiveVideoCall) {
                control.updateTabSelection(control.conferencePageId,
                                           GonnectWindow.PageType.Conference)
            } else if (count && isOnConferencePage && isConference) {
                control.updateTabSelection(control.callPageId,
                                           GonnectWindow.PageType.Call)
            } else if (!count && (isOnCallPage || isOnConferencePage)) {
                control.updateTabSelection(control.homePageId,
                                           GonnectWindow.PageType.Base)
            }
        }
    }

    function updateTabSelection(pageId : string, pageType : int) {
        // page{Id,Type} changes not as a result of tab bar clicks
        mainTabBar.selectedPageId = pageId
        mainTabBar.selectedPageType = pageType
    }

    function showPage(pageId : string) {
        if (control.previousPage) {
            control.previousPage.visible = false
        }

        const page = pageStack.getPage(pageId)
        if (page) {
            page.visible = true
            control.previousPage = page
        }

        control.updateTabSelection(pageId, GonnectWindow.PageType.Conference)
    }

    function openMeeting(meetingId : string, displayName : string, startFlags : int, callHistoryItem : variant) {
        control.updateTabSelection(control.conferencePageId,
                                   GonnectWindow.PageType.Conference)
        conferencePage.startConference(meetingId, displayName, startFlags, callHistoryItem)
    }

    function updateCallInForeground() {
        if (mainTabBar.selectedPageType === GonnectWindow.PageType.Conference) {
            GlobalCallState.callInForeground = conferencePage.iConferenceConnector
        } else if (mainTabBar.selectedPageType === GonnectWindow.PageType.Call) {
            const selectedCallItem = callPage.selectedCallItem
            if (selectedCallItem) {
                ViewHelper.setCallInForegroundByIds(selectedCallItem.accountId, selectedCallItem.callId)
            }
        }
    }

    function getPage(pageId : string) : Item {
        return pageStack.getPage(pageId)
    }

    function removePage(pageId : string) {
        const page = pageStack.getPage(pageId)
        page.isBeingDeleted = true
        pageModel.remove(page)
        page.model.removeAll()
        page.destroy()
        delete pageStack.getPage(pageId)

        mainTabBar.saveTabList()
    }

    function createPage(pageId : string, name : string, iconId : string, tab : variant) {
        const page = pages.base.createObject(pageStack,
                                           {
                                               pageId: pageId,
                                               name: name,
                                               iconId: iconId,
                                               tabButton: tab,
                                               editMode: true
                                           })
        if (page === null) {
            console.error(category, "could not create page component", pageId)
        }

        pageModel.add(page)
        pageStack.pages[pageId] = page

        page.writer.save()
        mainTabBar.saveTabList()
    }

    function loadPages() {
        pageReader.loadHomePage(control.homePageId)
        pageReader.loadDynamicPages()
        mainTabBar.sortTabList()
    }

    CommonPages {
        id: pages
    }

    property int notifications: pageModel.notifications + ChatConnectorManager.unreadNotificationsCount

    onNotificationsChanged: () => {
        SystemTrayMenu.setBadgeNumber(control.notifications)
    }

    PageModel {
        id: pageModel
    }

    PageReader {
        id: pageReader

        tabRoot: mainTabBar
        pageRoot: pageStack
        model: pageModel
    }

    Item {
        anchors.fill: parent

        Keys.onPressed: keyEvent => {
            if (keyEvent.key === Qt.Key_F11 || (keyEvent.key === Qt.Key_Escape && control.visibility === Window.FullScreen)) {

                // Toggle fullscreen
                keyEvent.accepted = true
                ViewHelper.toggleFullscreen()

            } else if ([Qt.Key_F, Qt.Key_K].includes(keyEvent.key) && (keyEvent.modifiers & Qt.ControlModifier)) {

                // Focus search field
                keyEvent.accepted = true
                ViewHelper.activateSearch()

            } else if (keyEvent.key === Qt.Key_V && (keyEvent.modifiers & Qt.ControlModifier)) {

                // Paste clipboard image content, if applicable
                if (ClipboardHelper.hasImage()) {
                    keyEvent.accepted = true

                    if (mainTabBar.selectedPageType === GonnectWindow.PageType.Chats) {
                        const page = control.getPage(mainTabBar.selectedPageId)
                        if (page) {
                            page.useImageFromClipboard()
                        }
                    }
                }

            } else if (keyEvent.key === Qt.Key_M
                       && (keyEvent.modifiers & Qt.ControlModifier)
                       && (keyEvent.modifiers & Qt.ShiftModifier)) {

                // Toggle Mute
                keyEvent.accepted = true
                GlobalMuteState.toggleMute()
            }
        }

        Drawer {
            id: topDrawer
            edge: Qt.TopEdge
            interactive: false
            parent: control.Overlay.overlay
            visible: !!topDrawerLoader.item
            topInset: control.shadowMargin
            topPadding: control.shadowMargin
            leftInset: topDrawer.leftPadding - topDrawer.borderPadding
            rightInset: topDrawer.rightPadding - topDrawer.borderPadding
            leftPadding: control.width / 2 - topDrawerLoader.implicitWidth / 2
            rightPadding: topDrawer.leftPadding

            readonly property int borderPadding: 20

            Material.accent: Theme.accentColor
            Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

            enter: Transition { NumberAnimation { duration: 1 } }
            exit: Transition { NumberAnimation { duration: 1 } }

            Overlay.modal: Component {
                Item {
                    Rectangle {
                        color: control.Material.backgroundDimColor
                        radius: 8
                        anchors {
                            fill: parent
                            margins: control.shadowMargin
                        }

                        Behavior on opacity { NumberAnimation { duration: 150 } }
                    }
                }
            }

            Component.onCompleted: () => {
                ViewHelper.topDrawer = topDrawer
            }

            readonly property alias loader: topDrawerLoader

            Loader {
                id: topDrawerLoader

                onItemChanged: () => topDrawerLoader.item?.forceActiveFocus()
            }
        }

        Rectangle {
            anchors.fill: parent
            color: Theme.backgroundSecondaryColor
        }

        MainTabBar {
            id: mainTabBar
            selectedPageId: control.homePageId
            selectedPageType: GonnectWindow.PageType.Base

            mainWindow: control

            hasActiveCall: callsModel.count > 0
            hasActiveUnfinishedCall: callsModel.unfinishedCount > 0
            hasActiveConference: conferencePage.iConferenceConnector.isInConference
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }

            onSelectedPageIdChanged: {
                control.showPage(selectedPageId)
            }

            onSelectedPageTypeChanged: {
                control.updateCallInForeground()
            }

            Component.onCompleted: {
                control.showPage(selectedPageId)
            }
        }

        ControlBar {
            id: controlBar
            visible: !Theme.useOwnDecoration
            showSearch: !SM.uiEditMode
            anchors {
                top: parent.top
                left: mainTabBar.right
                right: parent.right
            }

            tabRoot: mainTabBar
            pageRoot: pageStack
        }

        Item {
            id: pageStack
            anchors {
                left: mainTabBar.right
                right: parent.right
                top: controlBar.visible ? controlBar.bottom : parent.top
                bottom: bottomBar.visible ? bottomBar.top : parent.bottom
            }

            function getPage(pageId : string) : Item {
                switch (pageId) {
                    case control.homePageId:
                        return homePage
                    case control.callPageId:
                        return callPage
                    case control.chatsPageId:
                        return chatsPage
                    case control.conferencePageId:
                        return conferencePage
                    case control.settingsPageId:
                        return settingsPage
                    default:
                        return pageStack.pages[pageId]
                }
            }

            property var pages: ({})

            BasePage {
                id: homePage
                visible: false
                anchors.fill: parent

                pageId: control.homePageId
                name: qsTr("Home")
                iconId: "userHome"
                tabButton: mainTabBar.getTabById(control.homePageId)
            }

            Call {
                id: callPage
                visible: false
                anchors.fill: parent

                onSelectedCallItemChanged: () => control.updateCallInForeground()
            }

            Chats {
                id: chatsPage
                attachedData: mainTabBar.attachedData
                visible: false
                anchors.fill: parent
            }

            Conference {
                id: conferencePage
                visible: false
                anchors.fill: parent
            }

            SettingsPage {
                id: settingsPage
                visible: false
                anchors.fill: parent
            }
        }

        Item {
            id: bottomBar
            visible: true
            height: 35
            anchors {
                right: parent.right
                left: mainTabBar.right
                bottom: parent.bottom
            }

            TogglerList {
                id: togglerList
                visible: togglerList.count > 0
                clip: true
                anchors {
                    left: parent.left
                    right: rightRow.left
                    rightMargin: 24
                    verticalCenter: rightRow.verticalCenter
                }
            }

            Row {
                id: rightRow
                spacing: 10
                anchors {
                    right: parent.right
                    bottom: parent.bottom
                    rightMargin: 6
                }

                FirstAidButton {
                    id: firstAidButton
                    height: 42
                    z: 100000
                }
            }
        }
    }

    readonly property Popup globalEmojiPickerPopupItem: EmojiPickerPopup {
        id: globalEmojiPickerPopup
        Component.onCompleted: () => ViewHelper.globalEmojiPickerPopup = globalEmojiPickerPopup
    }

    readonly property Connections viewHelperConnections: Connections {
        target: ViewHelper
        function onUrlCopyDialogRequested(url, text) {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/UrlCopyDialog.qml", { url, text })
        }
        function onShowDialPad() {
            const item = drawerStackView.push("qrc:/qt/qml/base/ui/components/controls/DtmfDialer.qml")
            item.dialed.connect(button => console.log(category, "TODO: DIAL", button))
        }
        function onShowFirstAid() {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/FirstAid.qml")
        }
        function onShowChatUserSearchDialog(chatProvider : IChatProvider) {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/ChatUserSearch.qml", { chatProvider })
        }
        function onShowPublicRoomSearchDialog(chatProvider : IChatProvider) {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/PublicRoomSearch.qml", { chatProvider })
        }
        function onShowKnockRoomDialog(chatProvider : IChatProvider, roomId : string) {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/KnockChatRoom.qml",
                                 { chatProvider, roomId })
        }
        function onShowConferenceChat() {
            control.ensureVisible()
            control.updateTabSelection(control.conferencePageId, GonnectWindow.PageType.Conference)
        }
        function onShowChatRoom(provider : IChatProvider, roomId : string) {
            console.debug(category, `Showing room "${roomId}" for provider "${provider.id}" on page "${control.chatsPageId}"`)

            control.ensureVisible()
            control.showPage(control.chatsPageId, provider.id)

            const page = pageStack.getPage(control.chatsPageId)
            page.attachedData = provider
            page.showChatRoom(roomId)
        }
        function onShowCreateRoomDialog(chatProvider : IChatProvider, invitedUserIds : list<string>, name : string) {
            control.ensureVisible()
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/CreateChatRoom.qml",
                                 { chatProvider, userIds: invitedUserIds, roomName : name })
        }
        function onShowEditRoomDialog(chatProvider : IChatProvider, roomId : string) {
            control.ensureVisible()
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/EditChatRoom.qml",
                                 { chatProvider, roomId })
        }
        function onShowInviteUserToRoomDialog(chatProvider : IChatProvider, roomId : string) {
            control.ensureVisible()
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/InviteChatRoom.qml",
                                 { chatProvider, roomId })
        }
        function onShowEditMessageDialog(chatProvider : IChatProvider, roomId : string, messageId : string, content : string) {
            control.ensureVisible()
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/EditChatMessage.qml",
                                 { chatProvider, roomId, messageId, text: content })
        }
        function onShowLargeImage(imageFilePath : url) {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/LargeImage.qml", { source : imageFilePath })
        }
        function onShowLargeVideo(videoFilePath : url, fileName : string, fileSize : int, thumbnailFilePath : url) {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/LargeVideo.qml", {
                                     source : videoFilePath,
                                     fileName,
                                     fileSize,
                                     thumbnailFilePath
                                 })
        }
        function onShowStatusTextEditDialog() {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/EditStatusText.qml")
        }
        function onFullscreenToggle() {
            if (control.visibility === Window.FullScreen) {
                control.showNormal()
            } else {
                control.showFullScreen()
            }
        }
    }

    readonly property Popup mainDrawer: Popup {
        id: mainDrawer
        width: drawerStackView.currentItem
               ? Util.clamp(drawerStackView.currentItem.implicitWidth, 0.63 * control.width, control.width - 100)
               : 0
        height: drawerStackView.currentItem
                ? Util.clamp(drawerStackView.currentItem.implicitHeight, 0.63 * control.height, control.height - 100)
                : 0
        modal: true
        anchors.centerIn: parent
        background.visible: !drawerStackView.currentItem || !drawerStackView.currentItem.hidePopupBackground

        onClosed: () => drawerStackView.clear()

        StackView {
            id: drawerStackView
            anchors.fill: parent

            onEmptyChanged: () => {
                if (drawerStackView.empty) {
                    mainDrawer.close()
                } else {
                    mainDrawer.open()
                }
            }
        }
    }
}
