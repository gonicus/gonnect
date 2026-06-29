pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitWidth: 54 + 2 * 8
    implicitHeight: topMenuCol.implicitHeight

    LoggingCategory {
        id: category
        name: "gonnect.qml.MainTabBar"
        defaultLogLevel: LoggingCategory.Warning
    }

    property var mainWindow

    property int dynamicPageCount: 0
    property int dynamicPageLimit: 5

    property bool hasActiveCall
    property bool hasActiveUnfinishedCall
    property bool hasActiveConference

    property alias backgroundColor: filler.color

    Component {
        id: pageCreationWindowComponent

        PageCreationWindow {}
    }

    function openPageCreationDialog() {
        if (SM.uiHasActiveEditDialog) {
            return
        }

        // INFO: Artificial limitation to avoid tab bar clutter
        if (control.dynamicPageCount >= control.dynamicPageLimit) {
            return
        }

        const id = `page_${UISettings.generateUuid()}`
        const item = pageCreationWindowComponent.createObject(control, { pageId: id, newPage: true })
        if (item) {
            SM.uiHasActiveEditDialog = true

            item.accepted.connect((name, iconId) => {
                                      const tab = control.createTab(id, MainPageSelection.PageType.Base, iconId, name)
                                      control.mainWindow.createPage(id, name, iconId, tab)
                                  })
            item.show()
        }
    }

    function openPageEditDialog(id : string, newPage : bool) {
        if (SM.uiHasActiveEditDialog) {
            return
        }

        const page = control.mainWindow.getPage(id)
        if (!page) {
            console.error(category, "unable to find page with id", id)
            return
        }

        const item = pageCreationWindowComponent.createObject(control, { pageId: id, newPage: newPage })
        if (item) {
            SM.uiHasActiveEditDialog = true

            item.accepted.connect((name, iconId) => {
                                      page.iconId = iconId
                                      page.name = name

                                      // Update tab button
                                      if (page.tabButton) {
                                          page.tabButton.iconSource = Icons[iconId]
                                          page.tabButton.labelText = name
                                      }
                                  })
            item.prefill(page.iconId, page.name)
            item.show()
        }
    }

    function createTab(id : string, type : int, iconId : string, name : string) : variant {
        const iconPath = Icons[iconId]
        const tabButton = tabDelegate.createObject(topMenuCol,
                                                 {
                                                     pageId: id,
                                                     pageType: type,
                                                     iconSource: iconPath,
                                                     labelText: name,
                                                     disabledTooltipText: "",
                                                     isEnabled: true,
                                                     showActiveBorder: false,
                                                     attachedData: null
                                                 })
        if (tabButton === null) {
            console.error(category, "could not create tab button component")
            return tabButton
        }

        control.dynamicPageCount += 1
        return tabButton
    }

    function getTabById(id : string) : variant {
        return [...topMenuCol.children].find((button) => button.pageId === id);
    }

    function getTabList() {
        let tabOrder = []

        tabOrder.push(...topMenuCol.children)
        return tabOrder.filter((button) => button.pageId)
    }

    function saveTabList() {
        let tabNames = []
        let tabOrder = []

        tabOrder.push(...topMenuCol.children)
        tabOrder.forEach((button) => {
            if (button.pageId) {
                tabNames.push(button.pageId)
            }
        })

        if (tabNames.length > 0) {
            UISettings.setUISetting("generic", "tabBarOrder", tabNames.join(","))
        }
    }

    function sortTabList() {
        let tabList = UISettings.getUISetting("generic", "tabBarOrder", "").split(",")
        if (!tabList.length > 0) {
            return
        }

        let tabOrder = []
        let newOrder = []

        tabOrder.push(...topMenuCol.children)

        tabList.forEach((tabId) => {
            tabOrder.forEach((button) => {
                if (button.pageId && button.pageId === tabId) {
                    newOrder.push(button)
                }
            })
        })

        newOrder.forEach((button) => {
            button.parent = null
            button.visible = false

            button.parent = topMenuCol
            button.visible = true
        })

        control.saveTabList()
    }

    Rectangle {
        id: filler
        anchors.fill: parent
        color: control.Window.window?.active ? Theme.backgroundHeader : Theme.backgroundHeaderInactive
    }

    Component {
        id: tabDelegate

        Item {
            id: delg
            height: 54
            enabled: true
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property string pageId
            required property int pageType
            required property bool isEnabled
            required property bool showActiveBorder
            required property string labelText
            required property string disabledTooltipText
            required property string iconSource
            required property var attachedData

            property int notifications: (delg.attachedData && delg.attachedData instanceof IChatProvider)
                                        ? delg.attachedData.unreadNotificationsCount
                                        : 0
            property bool showNotificationBubble: delg.notifications > 0

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Selected tab")
            Accessible.description: qsTr("The currently selected tab")
            Accessible.focusable: true
            Accessible.onPressAction: () => delg.switchTab()

            function switchTab() {
                if (delg.isEnabled) {
                    SelectionState.selectedPage = {
                        id: delg.pageId,
                        type: delg.pageType,
                        attachedData: delg.attachedData ? (delg.attachedData as QtObject) : undefined
                    }
                }
            }

            Rectangle {
                id: hoverBackground
                visible: delgHoverHandler.hovered && (delg.isEnabled || SM.uiEditMode)
                radius: 8
                color: Theme.backgroundSecondaryColor
                anchors {
                    fill: parent
                    leftMargin: 8
                    rightMargin: 8
                }

                // Options
                Rectangle {
                    id: optionIndicator
                    visible: SM.uiEditMode
                    width: 16
                    height: 16
                    color: "transparent"
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 2
                    z: 1

                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Selected tab options")
                    Accessible.description: qsTr("The settings of the currently selected tab")
                    Accessible.focusable: true

                    IconLabel {
                        id: optionIcon
                        anchors.centerIn: parent
                        icon {
                            source: Icons.goDown
                            width: parent.width
                            height: parent.height
                        }

                        Accessible.ignored: true
                    }

                    MouseArea {
                        id: optionControl
                        parent: optionIndicator
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            optionMenu.x = delg.x + 15
                            optionMenu.y = delg.y + 15
                            optionMenu.selectedTabButton = delg
                            optionMenu.open()
                        }

                        Accessible.ignored: true
                    }
                }
            }

            Rectangle {
                id: activeBg
                radius: 8
                color: Theme.activeIndicatorColor
                anchors.fill: hoverBackground
                visible: delg.showActiveBorder

                SequentialAnimation {
                    running: activeBg.visible
                    loops: Animation.Infinite

                    NumberAnimation {
                        target: activeBg
                        property: "opacity"
                        from: 1.0
                        to: 0.0
                        duration: 2000
                    }
                    NumberAnimation {
                        target: activeBg
                        property: "opacity"
                        from: 0.0
                        to: 1.0
                        duration: 2000
                    }
                }

                Accessible.ignored: true
            }

            IconLabel {
                id: delgIcon
                anchors.centerIn: parent
                icon {
                    source: delg.iconSource
                    width: 32
                    height: 32
                    color: delg.isEnabled
                    ? Theme.primaryTextColor
                    : Theme.secondaryInactiveTextColor
                }

                Accessible.ignored: true
            }

            Rectangle {
                id: notificationBubbleBackground
                visible: notificationBubble.visible
                color: hoverBackground.visible ? hoverBackground.color : filler.color
                anchors.centerIn: notificationBubble
                width: notificationBubble.width + 4
                height: notificationBubbleBackground.width
                radius: notificationBubbleBackground.width / 2

                Accessible.ignored: true
            }

            Rectangle {
                id: notificationBubble
                visible: delg.showNotificationBubble
                color: Theme.redColor
                width: delg.notifications > notificationBubble.maxNotifications
                       ? 24 : 16
                height: 16
                radius: notificationBubble.height / 2
                anchors {
                    verticalCenter: delgIcon.bottom
                    horizontalCenter: delgIcon.right
                    verticalCenterOffset: -5
                    horizontalCenterOffset: -5
                }

                property int maxNotifications: 99

                Label {
                    id: notificationBubbleCount
                    color: Theme.foregroundWhiteColor
                    font.pixelSize: 12
                    text: delg.notifications > notificationBubble.maxNotifications
                          ? "99+" : delg.notifications.toString()
                    anchors.centerIn: parent
                }

                Accessible.ignored: true
            }

            ToolTip.text: delg.isEnabled ? delg.labelText : delg.disabledTooltipText
            ToolTip.visible: delgHoverHandler.hovered
            ToolTip.delay: Application.styleHints.mousePressAndHoldInterval
            ToolTip.toolTip.x: delg.x + delg.width
            ToolTip.toolTip.y: 9

            HoverHandler {
                id: delgHoverHandler
            }

            TapHandler {
                id: delgTapHandler
                onTapped: () => delg.switchTab()
            }
        }
    }

    Column {
        id: topMenuCol
        topPadding: 20
        spacing: 10
        anchors {
            left: parent.left
            right: parent.right
        }

        Repeater {
            id: menuRepeater
            delegate: tabDelegate
            model: {
                const baseModel = [
                    {
                        pageId: SelectionState.homePageId(),
                        pageType: MainPageSelection.PageType.Base,
                        iconSource: Icons.userHome,
                        labelText: qsTr("Home"),
                        disabledTooltipText: qsTr("Home"),
                        isEnabled: true,
                        showActiveBorder: false,
                        attachedData: null
                    }, {
                        pageId: SelectionState.conferencePageId(),
                        pageType: MainPageSelection.PageType.Conference,
                        iconSource: Icons.userGroupNew,
                        labelText: qsTr("Conference"),
                        disabledTooltipText: qsTr("No active conference"),
                        isEnabled: control.hasActiveConference,
                        showActiveBorder: control.hasActiveConference && SelectionState.selectedPage.id !== SelectionState.conferencePageId(),
                        attachedData: null
                    }, {
                        pageId: SelectionState.callPageId(),
                        pageType: MainPageSelection.PageType.Call,
                        iconSource: Icons.callStart,
                        labelText: qsTr("Call"),
                        disabledTooltipText: qsTr("No active call"),
                        isEnabled: control.hasActiveCall,
                        showActiveBorder: control.hasActiveUnfinishedCall && SelectionState.selectedPage.id !== SelectionState.callPageId(),
                        attachedData: null
                    }
                ].filter(item => ViewHelper.isJitsiAvailable || item.pageType !== MainPageSelection.PageType.Conference)

                if (ChatConnectorManager.isChatAvailable) {
                    for (const conn of ChatConnectorManager.chatConnectors) {
                        baseModel.push({
                                           pageId: SelectionState.chatsPageId(),
                                           pageType: MainPageSelection.PageType.Chats,
                                           iconSource: Icons.dialogMessages,
                                           labelText: conn.displayName,
                                           disabledTooltipText: qsTr("Chat not available"),
                                           isEnabled: conn.isConnected,
                                           showRedDot: false,
                                           attachedData: conn
                                       })
                    }
                }

                return baseModel
            }
        }

        Component.onCompleted: () => mainWindow.loadPages()
    }

    Column {
        id: bottomMenuCol
        bottomPadding: 20
        spacing: 10
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Repeater {
            id: bottomMenuRepeater
            delegate: tabDelegate
            model: [
                {
                    pageId: SelectionState.settingsPageId(),
                    pageType: MainPageSelection.PageType.Settings,
                    iconSource: Icons.settingsConfigure,
                    labelText: qsTr("Settings"),
                    disabledTooltipText: qsTr("Settings"),
                    isEnabled: true,
                    showActiveBorder: false,
                    attachedData: null
                }
            ]
        }
    }

    Menu {
        id: optionMenu
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property Item selectedTabButton: null

        QtObject {
            id: menuInternal

            property bool isFirst
            property bool isLast

            readonly property Connections menuConnections: Connections {
                target: optionMenu
                function onSelectedTabButtonChanged() { menuInternal.updateIndexes() }
                function onOpenedChanged() { menuInternal.updateIndexes() }
            }

            function updateIndexes() {
                const selButton = optionMenu.selectedTabButton
                if (!selButton) {
                    menuInternal.isFirst = false
                    menuInternal.isLast = false
                    return
                }

                const index = control.getTabList().findIndex(button => button.pageId === selButton.pageId)
                if (index >= 0) {
                    menuInternal.isFirst = index === 0
                    menuInternal.isLast = index === topMenuCol.children.length - 2
                } else {
                    menuInternal.isFirst = false
                    menuInternal.isLast = false
                }
            }
        }

        Action {
            id: moveUpAction
            text: qsTr("Move up")
            icon.source: Icons.arrowUp
            enabled: !menuInternal.isFirst

            onTriggered: () => moveUpAction.moveTabUp()

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Move tab up")
            Accessible.description: qsTr("Moves the currently selected tab up by one")
            Accessible.focusable: true
            Accessible.onPressAction: () => moveUpAction.moveTabUp()

            function moveTabUp() {
                if (optionMenu.selectedTabButton !== null) {
                    const newOrder = control.getTabList()
                    const index = newOrder.findIndex(button => button.pageId === optionMenu.selectedTabButton.pageId)

                    if (index > 0) {
                        let old = newOrder[index-1]
                        newOrder[index-1] = newOrder[index]
                        newOrder[index] = old

                        newOrder.forEach((button) => {
                            button.parent = null
                            button.visible = false

                            button.parent = topMenuCol
                            button.visible = true
                        })
                    }

                    control.saveTabList()
                }
            }
        }

        Action {
            id: moveDownAction
            text: qsTr("Move down")
            icon.source: Icons.arrowDown
            enabled: !menuInternal.isLast

            onTriggered: () => moveDownAction.moveTabDown()

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Move tab down")
            Accessible.description: qsTr("Moves the currently selected tab down by one")
            Accessible.focusable: true
            Accessible.onPressAction: () => moveDownAction.moveTabDown()

            function moveTabDown() {
                if (optionMenu.selectedTabButton !== null) {
                    const newOrder = control.getTabList()
                    const index = newOrder.findIndex(button => button.pageId === optionMenu.selectedTabButton.pageId)

                    if (index < newOrder.length-1) {
                        let old = newOrder[index+1]
                        newOrder[index+1] = newOrder[index]
                        newOrder[index] = old

                        newOrder.forEach((button) => {
                            button.parent = null
                            button.visible = false

                            button.parent = topMenuCol
                            button.visible = true
                        })
                    }

                    control.saveTabList()
                }
            }
        }

        Action {
            id: editPageAction
            text: qsTr("Edit")
            icon.source: Icons.editor
            enabled: optionMenu.selectedTabButton?.pageType === MainPageSelection.PageType.Base
                     && optionMenu.selectedTabButton?.pageId !== SelectionState.homePageId()
            onTriggered: () => control.openPageEditDialog(optionMenu.selectedTabButton.pageId, false)

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Edit page")
            Accessible.description: qsTr("Edit the currently selected dashboard page")
            Accessible.focusable: true
            Accessible.onPressAction: () => control.openPageEditDialog(optionMenu.selectedTabButton.pageId, false)
        }

        Action {
            id: deletePageAction
            text: qsTr("Delete")
            icon.source: Icons.editDelete
            enabled: optionMenu.selectedTabButton?.pageType === MainPageSelection.PageType.Base
                     && optionMenu.selectedTabButton?.pageId !== SelectionState.homePageId()

            onTriggered: () => deletePageAction.deletePage()

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Delete page")
            Accessible.description: qsTr("Delete the currently selected dashboard page")
            Accessible.focusable: true
            Accessible.onPressAction: () => deletePageAction.deletePage()

            function deletePage() {
                if (optionMenu.selectedTabButton !== null) {
                    let curIndex
                    let newIndex
                    let tabOrder = control.getTabList().filter((button) => button.isEnabled)

                    curIndex = tabOrder.findIndex(button => button.pageId === optionMenu.selectedTabButton.pageId)
                    if (curIndex > 0) {
                        newIndex = curIndex - 1
                    } else if (curIndex < tabOrder.length - 1) {
                        newIndex = curIndex + 1
                    }

                    mainWindow.showPage(tabOrder[newIndex].pageId,
                                                  tabOrder[newIndex].pageType)

                    mainWindow.removePage(optionMenu.selectedTabButton.pageId)
                    optionMenu.selectedTabButton.destroy()

                    control.dynamicPageCount -= 1
                    control.saveTabList()
                }
            }
        }
    }

    Rectangle {
        id: border
        color: Theme.borderColor
        width: 1
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
    }
}
