pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitHeight: 44

    required property bool showSearch

    required property var tabRoot
    required property var pageRoot

    function focusSearchBox() {
        searchField.giveFocus()
    }

    Row {
        id: editControls
        visible: !control.showSearch
        spacing: 15
        anchors.centerIn: parent

        Button {
            icon.source: Icons.listAdd
            height: 44
            text: qsTr("Add page")
            enabled: tabRoot.dynamicPageCount < tabRoot.dynamicPageLimit

            onPressed: {
                tabRoot.pageCreationDialog()
            }
        }

        Button {
            icon.source: Icons.viewLeftNew
            height: 44
            text: qsTr("Add widget")
            enabled: tabRoot.selectedPageType === GonnectWindow.PageType.Base

            onPressed: {
                let page = pageRoot.pages[tabRoot.selectedPageId]
                page.widgetCreationDialog()
            }
        }

        Button {
            highlighted: true
            Material.accent: Theme.greenColor
            icon.source: Icons.objectSelectSymbolic
            height: 44
            text: qsTr("Save")

            onClicked: {
                SM.setSaveDynamicUi(true)
            }
        }
    }

    SearchField {
        id: searchField
        visible: control.showSearch
        anchors.centerIn: parent

        Keys.onDownPressed: () => {
            resultPopup.initialKeyDown()
            resultPopup.forceActiveFocus()
        }
        Keys.onUpPressed: () => {
            resultPopup.initialKeyUp()
            resultPopup.forceActiveFocus()
        }
        Keys.onEscapePressed: () => {
            if (searchField.text.trim() === "") {
                const nextItem = searchField.nextItemInFocusChain(true)
                if (nextItem) {
                    nextItem.forceActiveFocus()
                }
            } else {
                searchField.text = ""
            }
        }
    }

    SearchResultPopup {
        id: resultPopup
        x: control.width / 2 - resultPopup.width / 2
        y: searchField.height + 12
        width: control.width * 0.75
        height: control.Window ? control.Window.height * 0.75 : 0
        topMargin: 12 + searchField.height
        searchText: searchField.text

        onPrimaryActionTriggered: () => {
            searchField.text = ""
        }

        onReturnFocus: () => control.focusSearchBox()
    }

    Row {
        spacing: 10
        anchors {
            right: parent.right
            rightMargin: 20
            verticalCenter: parent.verticalCenter
        }

        HeaderIconButton {
            id: burgerMenuButton
            iconSource: Icons.applicationMenu
            active: control.Window.active
            iconSize: 16
            anchors.verticalCenter: parent.verticalCenter
            onClicked: () => burgerMenu.open()

            BurgerMenu {
                id: burgerMenu
                x: burgerMenuButton.width - burgerMenu.width
                y: burgerMenuButton.height
            }
        }

        AvatarImage {
            id: avatarImage
            size: 28
            initials: ViewHelper.initials(ViewHelper.currentUserName)
            source: ViewHelper.currentUser?.hasAvatar ? ("file://" + ViewHelper.currentUser.avatarPath) : ""
            showBuddyStatus: ViewHelper.currentUser?.hasBuddyState ?? false
            buddyStatus: SIPBuddyState.UNKNOWN

            Component.onCompleted: () => {
                avatarImage.updateBuddyStatus()
            }

            function updateBuddyStatus() {
                avatarImage.buddyStatus = ViewHelper.currentUser?.hasBuddyState
                        ? SIPManager.buddyStatus(ViewHelper.currentUser.subscriptableNumber)
                        : SIPBuddyState.UNKNOWN
            }

            Connections {
                target: SIPManager
                enabled: ViewHelper.currentUser?.hasBuddyState ?? false
                function onBuddyStateChanged(url : string, status : int) {
                    avatarImage.updateBuddyStatus()
                }
            }
        }
    }
}
