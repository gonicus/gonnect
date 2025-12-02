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

    property var mainWindow

    property int dynamicPageCount: 0
    property int dynamicPageLimit: 5

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
        if (control.dynamicPageCount >= control.dynamicPageLimit) {
            return
        }

        const id = "page_"+UISettings.generateUuid()
        const type = GonnectWindow.PageType.Base

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
            required property bool showRedDot
            required property string labelText
            required property string disabledTooltipText
            required property string iconSource
            required property var attachedData

            readonly property bool isSelected: control.selectedPageId === delg.pageId

            Rectangle {
                id: hoverBackground
                visible: delg.isSelected
                         || (delgHoverHandler.hovered && (delg.isEnabled || SM.uiEditMode))
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
                    visible: SM.uiEditMode
                    width: 16
                    height: 16
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
                        cursorShape: Qt.PointingHandCursor

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
                    if (delg.isEnabled) {
                        control.selectedPageId = delg.pageId
                        control.selectedPageType = delg.pageType
                        control.attachedData = delg.attachedData
                    }
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
                        pageId: control.mainWindow.homePageId,
                        pageType: GonnectWindow.PageType.Base,
                        iconSource: Icons.userHome,
                        labelText: qsTr("Home"),
                        disabledTooltipText: qsTr("Home"),
                        isEnabled: true,
                        showRedDot: false,
                        attachedData: null
                    }, {
                        pageId: control.mainWindow.conferencePageId,
                        pageType: GonnectWindow.PageType.Conference,
                        iconSource: Icons.userGroupNew,
                        labelText: qsTr("Conference"),
                        disabledTooltipText: qsTr("No active conference"),
                        isEnabled: control.hasActiveConference,
                        showRedDot: false,
                        attachedData: null
                    }, {
                        pageId: control.mainWindow.callPageId,
                        pageType: GonnectWindow.PageType.Call,
                        iconSource: Icons.callStart,
                        labelText: qsTr("Call"),
                        disabledTooltipText: qsTr("No active call"),
                        isEnabled: control.hasActiveCall,
                        showRedDot: false,
                        attachedData: null
                    }
                ].filter(item => ViewHelper.isJitsiAvailable || item.pageType !== GonnectWindow.PageType.Conference)

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
                    pageId: control.mainWindow.settingsPageId,
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

        Action {
            text: qsTr("Move up")
            icon.source: "qrc:/icons/arrow-up.svg"

            onTriggered: {
                if (optionMenu.selectedTabButton !== null) {
                    let index
                    let newOrder = control.getTabList()

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
            icon.source: "qrc:/icons/arrow-down.svg"

            onTriggered: {
                if (optionMenu.selectedTabButton !== null) {
                    let index
                    let newOrder = control.getTabList()

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
            icon.source: "qrc:/icons/edit-delete.svg"
            enabled: optionMenu.selectedTabButton?.pageType === GonnectWindow.PageType.Base
                     && optionMenu.selectedTabButton?.pageId !== control.mainWindow.homePageId

            onTriggered: {
                if (optionMenu.selectedTabButton !== null) {
                    let curIndex
                    let newIndex
                    let tabOrder = control.getTabList().filter((button) => button.isEnabled)

                    if (optionMenu.selectedTabButton.isSelected) {
                        // If the actively selected button is deleted, move up/down
                        curIndex = tabOrder.findIndex(button => button.pageId === optionMenu.selectedTabButton.pageId)
                        if (curIndex > 0) {
                            newIndex = curIndex - 1
                        } else if (curIndex < tabOrder.length - 1) {
                            newIndex = curIndex + 1
                        }

                        mainWindow.updateTabSelection(tabOrder[newIndex].pageId,
                                                      tabOrder[newIndex].pageType)
                    }

                    mainWindow.removePage(optionMenu.selectedTabButton.pageId)
                    optionMenu.selectedTabButton.destroy()

                    control.dynamicPageCount -= 1

                    SM.uiDirtyState = true
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
