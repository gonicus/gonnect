pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitWidth: 54 + 2 * 8
    implicitHeight: topMenuCol.implicitHeight

    property string selectedPageId: ""
    property int selectedPageType: -1
    property var attachedData: null

    property string callPageId
    property string callsPageId
    property string chatsPageId
    property string conferencePageId
    property string settingsPageId
    property string defaultPageId

    property var mainWindow

    property int dynamicPageCount: 0
    property int dynamicPageLimit: 4

    property bool hasActiveCall
    property bool hasActiveConference

    property alias backgroundColor: filler.color

    Component {
        id: pageCreationWindowComponent
        PageCreationWindow {
            tabRoot: control
        }
    }

    function pageCreationDialog() {
        // INFO: Artificial limitation to avoid tab bar clutter
        if (control.dynamicPageCount > control.dynamicPageLimit) {
            return
        }

        let id = "page"+control.dynamicPageCount
        let type = GonnectWindow.PageType.Base

        pageCreationWindowComponent.createObject(control,
                                                 {
                                                     pageId: id,
                                                     pageType: type
                                                 }).show()
    }

    function createTab(id : string, type : int, icon : url, name : string) {
        // TODO: We should have a unified way in the config, not sometimes
        // path + file and other time just a file, especially considering the
        // path changes based on the theme, thus needlessly cluttering the config
        let iconPath
        if (!icon.toString().startsWith("qrc:/")) {
            if (Theme.isDarkMode) {
                iconPath = "qrc:/icons/dark/" + icon + ".svg"
            } else {
                iconPath = "qrc:/icons/" + icon + ".svg"
            }
        } else {
            // We already have a full path + file
            iconPath = icon
        }

        let tabButton = tabDelegate.createObject(topMenuCol,
                                                 {
                                                     pageId: id,
                                                     pageType: type,
                                                     iconSource: iconPath,
                                                     labelText: name,
                                                     disabledTooltipText: "",
                                                     isEnabled: true,
                                                     showRedDot: false,
                                                     attachedData: null
                                                 })
        if (tabButton === null) {
            console.log("Could not create tab button component")
            return
        }

        control.dynamicPageCount += 1
    }

    function saveTabList() {
        let tabOrder = []
        let tabButtons = topMenuCol.children

        for (let i = 0; i < tabButtons.length; i++) {
            let tabButton = tabButtons[i]

            tabOrder.push(tabButton.pageId)
        }

        if (tabOrder.length > 0) {
            UISettings.setUISetting("generic", "tabBarOrder", tabOrder.join(","))
        }
    }

    function sortTabList() {
        let tabList = UISettings.setUISetting("generic", "tabBarOrder", "")
        let tabOrder = tabList.split(",")
        if (!tabOrder.length > 0) {
            return
        }

        let tabButtons = topMenuCol.children

        for (let i = 0; i < tabButtons.length; i++) {
            let tabButton = tabButtons[i]

            tabButton.parent = null
            tabButton.visible = false
        }

        for (let j = 0; j < tabOrder.length; j++) {
            let tabId = tabOrder[j]

            for (let k = 0; k < tabButtons.length; k++) {
                let tabButton = tabButtons[k]

                if (tabButton.pageId === tabId) {
                    tabButton.parent = topMenuCol
                    tabButton.visible = true
                    break
                }
            }
        }
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
            enabled: delg.isEnabled
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property string pageId
            required property int pageType
            required property bool isEnabled
            required property bool showRedDot
            required property string labelText
            required property string disabledTooltipText
            required property string iconSource
            required property var attachedData

            readonly property bool isSelected: control.selectedPageId === delg.pageId

            Rectangle {
                id: hoverBackground
                visible: delg.isSelected || (delg.isEnabled && delgHoverHandler.hovered)
                radius: 4
                color: Theme.backgroundSecondaryColor
                anchors {
                    fill: parent
                    leftMargin: 8
                    rightMargin: 8
                }

                // Options
                Rectangle {
                    id: optionIndicator
                    visible: SM.uiEditMode && delg.pageType === GonnectWindow.PageType.Base
                    width: 10
                    height: 10
                    color: "transparent"
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 2
                    z: 1

                    IconLabel {
                        id: removeIcon
                        anchors.centerIn: parent
                        icon {
                            source: Icons.goDown
                            width: parent.width
                            height: parent.height
                        }
                    }

                    MouseArea {
                        id: optionControl
                        parent: optionIndicator
                        anchors.fill: parent
                        cursorShape: Qt.CrossCursor

                        onClicked: {
                            optionMenu.x = delg.x + 15
                            optionMenu.y = delg.y + 15
                            optionMenu.selectedTabButton = delg
                            optionMenu.open()
                        }
                    }
                }
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
            }

            Rectangle {
                id: redDotBackground
                visible: redDot.visible
                color: hoverBackground.visible ? hoverBackground.color : filler.color
                anchors.centerIn: redDot
                width: redDot.width + 4
                height: redDotBackground.width
                radius: redDotBackground.width / 2
            }

            Rectangle {
                id: redDot
                visible: delg.showRedDot
                color: Theme.redColor
                width: 6
                height: redDot.width
                radius: redDot.width / 2
                anchors {
                    verticalCenter: delgIcon.top
                    horizontalCenter: delgIcon.right
                    verticalCenterOffset: +5
                    horizontalCenterOffset: -5
                }
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
                onTapped: () => {
                    control.selectedPageId = delg.pageId
                    control.selectedPageType = delg.pageType
                    control.attachedData = delg.attachedData
                }
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
                        pageId: control.callsPageId,
                        pageType: GonnectWindow.PageType.Calls,
                        iconSource: Icons.userHome,
                        labelText: qsTr("Home"),
                        disabledTooltipText: qsTr("Home"),
                        isEnabled: true,
                        showRedDot: false,
                        attachedData: null
                    }, {
                        pageId: control.conferencePageId,
                        pageType: GonnectWindow.PageType.Conference,
                        iconSource: Icons.userGroupNew,
                        labelText: qsTr("Conference"),
                        disabledTooltipText: qsTr("No active conference"),
                        isEnabled: control.hasActiveConference,
                        showRedDot: false,
                        attachedData: null
                    }, {
                        pageId: control.callPageId,
                        pageType: GonnectWindow.PageType.Call,
                        iconSource: Icons.callStart,
                        labelText: qsTr("Call"),
                        disabledTooltipText: qsTr("No active call"),
                        isEnabled: control.hasActiveCall,
                        showRedDot: false,
                        attachedData: null
                    }
                ].filter(item => ViewHelper.isJitsiAvailable || item.pageType !== GonnectWindow.PageType.Conference)

                if (ChatConnectorManager.isJsChatAvailable) {
                    const chatConnectors = ChatConnectorManager.jsChatConnectors

                    for (const connector of chatConnectors) {
                        baseModel.push({
                            pageId: control.chatsPageId,
                            pageType: GonnectWindow.PageType.Chats,
                            iconSource: Icons.dialogMessages,
                            labelText: connector.displayName,
                            disabledTooltipText: qsTr("Chat not available"),
                            isEnabled: true,
                            showRedDot: false,
                            attachedData: connector
                        })
                    }
                }

                return baseModel
            }
        }

        Component.onCompleted: {
            mainWindow.loadPages()
        }
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
                    pageId: control.settingsPageId,
                    pageType: GonnectWindow.PageType.Settings,
                    iconSource: Icons.settingsConfigure,
                    labelText: qsTr("Settings"),
                    disabledTooltipText: qsTr("Settings"),
                    isEnabled: true,
                    showRedDot: false,
                    attachedData: null
                }
            ]
        }
    }

    Menu {
        id: optionMenu
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property var selectedTabButton: null

        // TODO: baseModel items behave differently

        Action {
            text: qsTr("Move up")

            onTriggered: {
                if (optionMenu.selectedTabButton !== null) {
                    let index
                    let newOrder = []

                    newOrder.push(...topMenuCol.children)
                    newOrder = newOrder.filter((button) => button.pageId)
                    index = newOrder.findIndex(button => button.pageId === optionMenu.selectedTabButton.pageId)

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
                }
            }
        }

        Action {
            text: qsTr("Move down")

            onTriggered: {
                if (optionMenu.selectedTabButton !== null) {
                    let index
                    let newOrder = []

                    newOrder.push(...topMenuCol.children)
                    newOrder = newOrder.filter((button) => button.pageId)
                    index = newOrder.findIndex(button => button.pageId === optionMenu.selectedTabButton.pageId)

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
                }
            }
        }

        Action {
            text: qsTr("Delete")

            onTriggered: {
                if (optionMenu.selectedTabButton !== null) {
                    mainWindow.removePage(optionMenu.selectedTabButton.pageId)
                    optionMenu.selectedTabButton.destroy()

                    control.dynamicPageCount -= 1

                    mainWindow.updateTabSelection(control.callsPageId,
                                                  GonnectWindow.PageType.Calls)
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
