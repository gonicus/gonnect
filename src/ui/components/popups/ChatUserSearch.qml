pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 800
    implicitHeight: 600

    property alias chatProvider: chatUserSearchModel.chatProvider

    QtObject {
        id: internal

        property Item selectedItem

        readonly property Timer searchDebouncer: Timer {
            id: searchDebounceTimer
            interval: 200

            onTriggered: () => {
                chatUserSearchModel.searchPhrase = searchTextField.text.trim()
            }
        }

        readonly property Timer focusTimer: Timer {
            interval: 100
            running: true

            onTriggered: () => searchTextField.forceActiveFocus()
        }

        function selectUser() {
            if (internal.selectedItem) {
                const userId = internal.selectedItem.id
                const roomId = control.chatProvider.chatRoomIdForUser(userId)

                control.StackView.view.popCurrentItem(StackView.Immediate)

                if (roomId) {
                    // Room already exists: show room
                    ViewHelper.showChatRoom(control.chatProvider, roomId)
                } else {
                    // Room does not exist: show dialog to create room and invite user
                    ViewHelper.showCreateRoomDialog(control.chatProvider, [ userId ])
                }
            }
        }
    }

    Keys.onReturnPressed: () => internal.selectUser()
    Keys.onEnterPressed:  () => internal.selectUser()

    Keys.onDownPressed: () => {
        if (internal.selectedItem) {
            const newIdx = internal.selectedItem.index + 1
            internal.selectedItem = searchResultListView.itemAtIndex(newIdx < searchResultListView.count
                                                                     ? newIdx
                                                                     : 0)
        } else {
            internal.selectedItem = searchResultListView.itemAtIndex(0)
        }
    }

    Keys.onUpPressed: () => {
        const highestIdx = searchResultListView.count - 1

        if (internal.selectedItem) {
            const newIdx = internal.selectedItem.index - 1
            internal.selectedItem = searchResultListView.itemAtIndex(newIdx >= 0 ? newIdx : highestIdx)
        } else {
            internal.selectedItem = searchResultListView.itemAtIndex(highestIdx)
        }
    }

    TextField {
        id: searchTextField
        placeholderText: qsTr("Search for users...")
        anchors {
            top: parent.top
            left: parent.left
            right: closeButton.left
            margins: 20
        }

        onTextEdited: () => searchDebounceTimer.start()
    }

    HeaderIconButton {
        id: closeButton
        iconSource: Icons.mobileCloseApp
        anchors {
            top: parent.top
            right: parent.right
        }

        onClicked: () => control.StackView.view.popCurrentItem(StackView.Immediate)
    }

    ListView {
        id: searchResultListView
        clip: true
        anchors {
            top: searchTextField.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
            margins: 20
        }
        model: ChatUserSearchModel {
            id: chatUserSearchModel
        }
        delegate: Item {
            id: delg

            required property int index
            required property string id
            required property string name
            required property string avatarPath
            required property bool hasPresenceState
            required property int presenceState

            height: 50
            anchors {
                left: parent?.left
                right: parent?.right
            }

            Rectangle {
                id: background
                visible: internal.selectedItem === delg
                color: Theme.backgroundOffsetHoveredColor
                anchors.fill: parent
                radius: 4
            }

            AvatarImage {
                id: avatarImage
                initials: ViewHelper.initials(delg.name)
                source: delg.avatarPath
                showPresenceStatus: delg.hasPresenceState
                presenceStatus: delg.presenceState
                size: 40
                indicatorComponent: Component { ChatUserPresenceStatusIndicator {} }
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    margins: 10
                }
            }

            Label {
                text: delg.name
                elide: Text.ElideRight
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: avatarImage.right
                    right: parent.right
                    leftMargin: 10
                    rightMargin: 20
                }
            }

            HoverHandler {
                id: delegateHoverHandler
                onHoveredChanged: () => {
                    if (delegateHoverHandler.hovered) {
                        internal.selectedItem = delg
                    } else if (internal.selectedItem === delg) {
                        internal.selectedItem = null
                    }
                }
            }

            TapHandler {
                onTapped: () => internal.selectUser()
            }
        }
    }
}
