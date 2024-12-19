pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import Qt5Compat.GraphicalEffects
import base

BaseWindow {
    id: control
    width: 910
    height: 554
    minimumWidth: 910
    minimumHeight: 554
    title: "GOnnect"
    resizable: true

    signal showHistory
    signal showSettings
    signal showCalls
    signal showShortcuts
    signal showAbout

    onActiveChanged: () => {
        if (control.active) {
            SIPCallManager.resetMissedCalls()
        }
    }

    function activateSearch() {
        searchBox.activate()
        SIPCallManager.resetMissedCalls()
    }

    Component.onCompleted: searchBox.activate()

    Item {
        anchors.fill: parent

        states: [
            State {
                when: !SIPAccountManager.sipRegistered
                PropertyChanges {
                    regularContainer.enabled: false
                    sipDisconnectedBlocker.visible: true
                    retryTimer.running: true
                }
            }
        ]

        Item {
            id: regularContainer
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: bottomStatusBar.visible ? bottomStatusBar.top : parent.bottom
            }

            SearchDial {
                id: searchBox

                anchors {
                    top: parent.top
                    left: parent.left
                    right: burgerMenuButton.left
                    margins: 20
                }

                onNumberSelected: (number, contactId, preferredIdentity) => {
                    SIPCallManager.call("account0", number, contactId, preferredIdentity);
                }
                onEscapePressed: () => control.close()
            }

            Rectangle {
                id: burgerMenuButton
                width: burgerMenuButton.height
                radius: searchBox.radius
                color: burgerMenuHoveredHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor
                anchors {
                    top: searchBox.top
                    bottom: searchBox.bottom
                    right: parent.right
                    rightMargin: 20
                }

                Behavior on color { ColorAnimation {} }

                HoverHandler {
                    id: burgerMenuHoveredHandler
                }

                TapHandler {
                    onTapped: () => burgerMenu.open()
                }

                IconLabel {
                    id: burgerMenuIcon
                    anchors.centerIn: parent
                    icon.source: Icons.applicationMenu
                }

                Menu {
                    id: burgerMenu
                    y: burgerMenuButton.height

                    Action {
                        text: qsTr("History...")
                        onTriggered: () => control.showHistory()
                    }
                    Action {
                        text: qsTr("Settings...")
                        onTriggered: () => control.showSettings()
                    }
                    Action {
                        text: qsTr("Shortcuts...")
                        enabled: SM.globalShortcutsSupported
                        onTriggered: () => control.showShortcuts()
                    }
                    MenuSeparator {}

                    // Togglers will be inserted here by instantiator

                    MenuSeparator {
                        visible: burgerMenuTogglerInstantiator.count > 0
                    }

                    Action {
                        text: qsTr("About...")
                        onTriggered: () => control.showAbout()
                    }
                    Action {
                        text: qsTr("Quit")
                        onTriggered: () => ViewHelper.quitApplication()
                    }

                    Instantiator {
                        id: burgerMenuTogglerInstantiator
                        model: TogglerProxyModel {
                            displayFilter: Toggler.MENU
                            TogglerModel {}
                        }
                        delegate: Action {
                            id: delg
                            text: delg.name
                            enabled: !delg.isBusy
                            icon.color: delg.isBusy ? Theme.secondaryTextColor : Theme.primaryTextColor
                            icon.source: delg.isBusy
                                         ? Icons.viewRefresh
                                         : (delg.isActive
                                            ? Icons.checkbox
                                            : '')

                            required property string id
                            required property string name
                            required property bool isActive
                            required property bool isBusy

                            onTriggered: () => TogglerManager.toggleToggler(delg.id)
                        }

                        onObjectAdded: (index, object) => burgerMenu.insertAction(index + 5, object)
                        onObjectRemoved: (index, object) => burgerMenu.removeAction(object)
                    }
                }
            }

            Rectangle {
                id: separatorLine
                height: 1
                color: Theme.borderColor
                anchors {
                    top: searchBox.bottom
                    left: parent.left
                    right: parent.right
                    margins: 20
                }
            }

            HistoryList {
                id: shortHistoryList
                limit: 10
                anchors {
                    top: separatorLine.bottom
                    left: parent.left
                    right: verticalSeparatorLine.visible ? verticalSeparatorLine.left : parent.right
                    bottom: showHistoryButton.visible ? showHistoryButton.top : parent.bottom
                    leftMargin: 20
                    rightMargin: 20
                    bottomMargin: 20
                }
            }

            Label {
                id: showHistoryButton
                visible: shortHistoryList.hasPastCalls
                color: showHistoryButtonHoverHandler.hovered ? Theme.primaryTextColor : Theme.secondaryTextColor
                text: "🕓  " + qsTr('Show complete history...')
                anchors {
                    horizontalCenter: shortHistoryList.horizontalCenter
                    bottom: parent.bottom
                    bottomMargin: 10
                }

                Behavior on color { ColorAnimation {} }

                HoverHandler {
                    id: showHistoryButtonHoverHandler
                }

                TapHandler {
                    onTapped: () => control.showHistory()
                }
            }

            Rectangle {
                id: verticalSeparatorLine
                visible: favList.visible
                width: 1
                color: Theme.borderColor
                anchors {
                    top: separatorLine.bottom
                    bottom: parent.bottom
                    right: favList.left
                    margins: 20
                }
            }

            FavoritesList {
                id: favList
                visible: favList.count > 0
                width: 180
                anchors {
                    top: separatorLine.bottom
                    right: parent.right
                    bottom: parent.bottom
                    rightMargin: 20
                    bottomMargin: 20
                }
            }
        }

        BottomStatusBar {
            id: bottomStatusBar
            visible: bottomStatusBar.count > 0
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
        }

        Item {
            id: sipDisconnectedBlocker
            anchors.fill: parent
            visible: false

            onVisibleChanged: () => console.log("Changed visibility of disconnected indicator to", sipDisconnectedBlocker.visible)

            Timer {
                id: retryTimer
                running: false
                repeat: true
                interval: 1000

                property int remainingSeconds: SIPAccountManager.sipRegisterRetryInterval
                property date started

                onRunningChanged: () => {
                    retryTimer.remainingSeconds = SIPAccountManager.sipRegisterRetryInterval
                    retryTimer.started = new Date()
                }

                onTriggered: () => {
                    const INTERVALL = SIPAccountManager.sipRegisterRetryInterval
                    const diff = (Date.now() - retryTimer.started.getTime()) / 1000
                    if (diff > INTERVALL) {
                        retryTimer.remainingSeconds = INTERVALL
                        retryTimer.started = new Date()
                    } else {
                        retryTimer.remainingSeconds = INTERVALL - diff
                    }
                }
            }

            Rectangle {
                color: Theme.backgroundColor
                anchors.fill: parent
                opacity: 0.8
            }

            Column {
                anchors.centerIn: parent
                spacing: 20

                BusyIndicator {
                    running: sipDisconnectedBlocker.visible
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Label {
                    text: qsTr("Cannot connect to SIP server, retrying in %n second(s)...", "", retryTimer.remainingSeconds)
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 20
                }
            }
        }
    }
}
