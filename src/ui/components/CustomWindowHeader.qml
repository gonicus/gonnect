pragma ComponentBehavior: Bound

import QtQuick
import base

Rectangle {
    id: control
    color: Theme.backgroundSecondaryColor
    height: 44
    radius: 8
    anchors {
        top: parent?.top
        left: parent?.left
        right: parent?.right
    }

    signal toggleMaximized

    readonly property var window: control.Window.window
    readonly property bool active: control.Window.window?.active ?? false

    property int mainBarWidth: 0
    property color mainBarColor

    function focusSearchBox() {
        searchField.giveFocus()
    }

    Rectangle {
        id: mimicRect
        visible: false
        width: 96
        radius: parent.radius
        color: control.Window.window?.active ? Theme.backgroundHeader : Theme.backgroundHeaderInactive
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
        }

        Rectangle {
            color: mimicRect.color
            width: mimicRect.radius
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
            }
        }

        Rectangle {
            color: mimicRect.color
            width: mimicRect.radius
            height: mimicRect.radius
            anchors {
                bottom: parent.bottom
                left: parent.left
            }
        }

        Rectangle {
            color: Theme.borderColor
            width: 1
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
            }
        }
    }

    // This rectangle makes the bottom rounded corners of rect straight
    Rectangle {
        height: control.radius
        color: control.color
        anchors {
            left: control.left
            right: control.right
            bottom: control.bottom
        }
    }

    DragHandler {
        id: systemDragHandler
        target: null
        onActiveChanged: () => {
            if (systemDragHandler.active) {
                control.window.startSystemMove()
            }
        }
    }

    Rectangle {
        id: tabBarSimulator
        width: control.mainBarWidth
        topLeftRadius: control.radius
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
        }

        color: control.mainBarColor

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

    TapHandler {
        onDoubleTapped: () => control.toggleMaximized()
    }

    Image {
        width: 24
        height: 24
        source: "qrc:/icons/gonnect.svg"
        sourceSize.width: 24
        sourceSize.height: 24
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 10
        }
    }

    SearchField {
        id: searchField
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
            rightMargin: 8
            verticalCenter: parent.verticalCenter
        }

        HeaderIconButton {
            id: burgerMenuButton
            iconSource: Icons.applicationMenu
            active: control.active
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

        HeaderIconButton {
            iconSource: Icons.mobileCloseApp
            active: control.active
            iconSize: 10
            anchors.verticalCenter: parent.verticalCenter
            onClicked: () => control.window?.close()
        }
    }
}
