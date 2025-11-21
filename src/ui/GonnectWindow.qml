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

    enum PageId {
        Activity,
        Calls,
        Call,
        Conference,
        Chats,
        Settings
    }

    readonly property CallsModel globalCallsModel: CallsModel {
        id: callsModel
    }

    readonly property Connections globalStateConnections: Connections {
        target: GlobalCallState

        function onCallStarted(isConference : bool) {
            control.showPage(isConference ? GonnectWindow.PageId.Conference : GonnectWindow.PageId.Call)
        }

        function onCallEnded(isConference : bool) {
            const count = GlobalCallState.activeCallsCount
            const isOnCallPage = mainTabBar.selectedPageId === GonnectWindow.PageId.Call
            const isOnConferencePage = mainTabBar.selectedPageId === GonnectWindow.PageId.Conference

            if (count && isOnCallPage && ViewHelper.isActiveVideoCall) {
                control.showPage(GonnectWindow.PageId.Conference)
            } else if (count && isOnConferencePage && isConference) {
                control.showPage(GonnectWindow.PageId.Call)
            } else if (!count && (isOnCallPage || isOnConferencePage)) {
                control.showPage(GonnectWindow.PageId.Calls)
            }
        }
    }

    function showPage(pageId : int) {
        mainTabBar.selectedPageId = pageId
    }

    function openMeeting(meetingId : string, displayName : string, startFlags : int, callHistoryItem : variant) {
        control.showPage(GonnectWindow.PageId.Conference)
        conferencePage.startConference(meetingId, displayName, startFlags, callHistoryItem)
    }

    function updateCallInForeground() {
        if (mainTabBar.selectedPageId === GonnectWindow.PageId.Conference) {
            GlobalCallState.callInForeground = conferencePage.iConferenceConnector
        } else if (mainTabBar.selectedPageId === GonnectWindow.PageId.Call) {
            const selectedCallItem = callPage.selectedCallItem
            if (selectedCallItem) {
                ViewHelper.setCallInForegroundByIds(selectedCallItem.accountId, selectedCallItem.callId)
            }
        }
    }

    Item {
        anchors.fill: parent

        Keys.onPressed: keyEvent => {
            if (keyEvent.key === Qt.Key_F11 || (keyEvent.key === Qt.Key_Escape && control.visibility === Window.FullScreen)) {

                // Toggle fullscreen
                keyEvent.accepted = true
                ViewHelper.toggleFullscreen()

            } else if (keyEvent.key === Qt.Key_F && (keyEvent.modifiers & Qt.ControlModifier)) {

                // Focus search field
                keyEvent.accepted = true
                ViewHelper.activateSearch()

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
            }
        }

        Rectangle {
            anchors.fill: parent
            color: Theme.backgroundSecondaryColor
        }

        MainTabBar {
            id: mainTabBar
            selectedPageId: GonnectWindow.PageId.Calls
            hasActiveCall: callsModel.count > 0
            hasActiveConference: conferencePage.iConferenceConnector.isInConference
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }

            onSelectedPageIdChanged: () => control.updateCallInForeground()
        }

        ControlBar {
            id: controlBar
            visible: !Theme.useOwnDecoration
            anchors {
                left: mainTabBar.right
                right: parent.right
                top: parent.top
            }
        }

        Item {
            id: pageStack
            anchors {
                left: mainTabBar.right
                right: parent.right
                top: controlBar.visible ? controlBar.bottom : parent.top
                bottom: parent.bottom
            }

            onSelectedPageChanged: () => {
                // Forward attached data from MainTabBar element
                const page = pageStack.selectedPage
                if (page && page.hasOwnProperty("attachedData")) {
                    page.attachedData = mainTabBar.attachedData
                }
            }

            readonly property Item selectedPage: {
                for (const child of pageStack.children) {
                    if (child.visible) {
                        return child
                    }
                }
                return null
            }

            states: [
                State {
                    when: mainTabBar.selectedPageId < 0
                    PropertyChanges {
                        defaultPage.visible: true
                    }
                },
                State {
                    when: mainTabBar.selectedPageId === GonnectWindow.PageId.Activity
                    PropertyChanges {
                        activityPage.visible: true
                    }
                },
                State {
                    when: mainTabBar.selectedPageId === GonnectWindow.PageId.Settings
                    PropertyChanges {
                        settingsPage.visible: true
                    }
                },
                State {
                    when: mainTabBar.selectedPageId === GonnectWindow.PageId.Call
                    PropertyChanges {
                        callPage.visible: true
                    }
                },
                State {
                    when: mainTabBar.selectedPageId === GonnectWindow.PageId.Calls
                    PropertyChanges {
                        callsPage.visible: true
                    }
                },
                State {
                    when: mainTabBar.selectedPageId === GonnectWindow.PageId.Conference
                    PropertyChanges {
                        conferencePage.visible: true
                    }
                },
                State {
                    when: mainTabBar.selectedPageId === GonnectWindow.PageId.Chats
                    PropertyChanges {
                        chatsPage.visible: true
                    }
                }
            ]

            // Default {
            Default {
                id: defaultPage
                visible: false
                anchors.fill: parent
            }
            Activity {
                id: activityPage
                visible: false
                anchors.fill: parent
            }
            Call {
                id: callPage
                visible: false
                anchors.fill: parent

                onSelectedCallItemChanged: () => control.updateCallInForeground()
            }
            Calls {
                id: callsPage
                visible: false
                anchors.fill: parent
            }
            Conference {
                id: conferencePage
                visible: false
                anchors.fill: parent
            }
            Chats {
                id: chatsPage
                visible: false
                anchors.fill: parent
            }
            SettingsPage {
                id: settingsPage
                visible: false
                anchors.fill: parent
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
            control.showPage(GonnectWindow.PageId.Conference)
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
