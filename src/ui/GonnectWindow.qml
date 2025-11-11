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
        if (previousPage) {
            previousPage.visible = false
        }

        let page = pageStack.getPage(pageId)
        if (page) {
            page.visible = true
            previousPage = page
        }
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

    function removePage(pageId : string) {
        let page = pageStack.getPage(pageId)
        pageModel.remove(page)
        page.model.removeAll()
        page.destroy()
        delete pageStack.getPage(pageId)
    }

    function createPage(pageId : string, icon : url, name : string) {
        let page = pages.base.createObject(pageStack,
                                           {
                                               pageId: pageId,
                                               name: name,
                                               icon: icon,
                                               editMode: true
                                           })
        if (page === null) {
            console.log("Could not create page component", pageId)
        }

        pageModel.add(page)
        pageStack.pages[pageId] = page
    }

    function loadPages() {
        pageReader.loadHomePage(control.homePageId)
        pageReader.loadDynamicPages()

        mainTabBar.sortTabList()
    }

    readonly property Connections dynamicUiConnections: Connections {
        target: SM
        function onUiSaveStateChanged() {
            if (SM.uiSaveState) {
                console.log("Writing dynamic UI state to disk")

                // Home page
                let homePage = pageStack.getPage(control.homePageId)
                homePage.writer.save()
                UISettings.setUISetting("generic", "gonnectGridWidth", homePage.gridWidth)
                UISettings.setUISetting("generic", "gonnectGridHeight", homePage.gridHeight)

                // Dynamic pages & widgets
                let pages = pageModel.items()
                for (const page of pages) {
                    page.writer.save()
                }

                // Tabs
                mainTabBar.saveTabList()

                SM.setUiSaveState(false)
                SM.setUiDirtyState(false)
            }
        }
    }

    CommonPages {
        id: pages
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
                keyEvent.accepted = true
                ViewHelper.toggleFullscreen()
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

            function getPage(pageId : string) : variant {
                switch (pageId) {
                    case control.homePageId:
                        return homePage
                    case control.callPageId:
                        return callPage
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
                icon: Icons.userHome

                oldGridWidth: UISettings.getUISetting("generic", "gonnectGridWidth", 0)
                oldGridHeight: UISettings.getUISetting("generic", "gonnectGridHeight", 0)
            }

            Call {
                id: callPage
                visible: false
                anchors.fill: parent

                onSelectedCallItemChanged: () => control.updateCallInForeground()
            }

            Chats {
                id: chatsPage
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

        Rectangle {
            id: bottomBar
            visible: mainTabBar.selectedPageType === GonnectWindow.PageType.Base
            color: "transparent"
            height: 35
            anchors {
                right: parent.right
                left: parent.left
                bottom: parent.bottom
            }

            Row {
                spacing: 10
                anchors {
                    right: parent.right
                    bottom: parent.bottom

                    topMargin: 6
                    bottomMargin: 6
                    leftMargin: 12
                    rightMargin: 12
                }

                FirstAidButton {
                    id: firstAidButton
                    z: 100000
                }
            }
        }
    }

    readonly property Connections viewHelperConnections: Connections {
        target: ViewHelper
        function onShowDialPad() {
            const item = drawerStackView.push("qrc:/qt/qml/base/ui/components/controls/DtmfDialer.qml")
            item.dialed.connect(button => console.log("TODO: DIAL", button))
        }
        function onShowFirstAid() {
            drawerStackView.push("qrc:/qt/qml/base/ui/components/popups/FirstAid.qml")
        }
        function onShowConferenceChat() {
            control.ensureVisible()
            control.updateTabSelection(control.conferencePageId, GonnectWindow.PageType.Conference)
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
        width: drawerStackView.currentItem  ? Math.min(0.63 * control.width,  drawerStackView.currentItem?.implicitWidth)  : (0.63 * control.width)
        height: drawerStackView.currentItem ? Math.min(0.63 * control.height, drawerStackView.currentItem?.implicitHeight) : (0.63 * control.height)
        modal: true
        anchors.centerIn: parent

        onClosed: drawerStackView.clear()

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
