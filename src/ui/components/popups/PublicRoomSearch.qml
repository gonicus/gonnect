pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 800
    implicitHeight: 600

    property alias chatProvider: chatRoomSearchModel.chatProvider

    QtObject {
        id: internal

        property Item selectedItem

        readonly property Timer searchDebouncer: Timer {
            id: searchDebounceTimer
            interval: 200

            onTriggered: () => {
                const phrase = searchTextField.text.trim()
                if (phrase.length >= 3) {
                    chatRoomSearchModel.searchPhrase = phrase
                }
            }
        }

        readonly property Timer focusTimer: Timer {
            interval: 100
            running: true

            onTriggered: () => searchTextField.forceActiveFocus()
        }

        function selectRoom() {
            if (internal.selectedItem) {
                const roomId = internal.selectedItem.id
                const joinRule = internal.selectedItem.joinRule

                control.StackView.view.popCurrentItem(StackView.Immediate)

                if (joinRule === IChatRoom.JoinRule.Knock) {
                    ViewHelper.showKnockRoomDialog(control.chatProvider, roomId)
                } else if (joinRule === IChatRoom.JoinRule.Public) {
                    control.chatProvider.joinRoomRequest(roomId)
                } else {
                    console.error(`Cannot handle unknown join rule ${joinRule} for chat room ${roomId}`)
                }
            }
        }
    }

    Keys.onReturnPressed: () => internal.selectRoom()
    Keys.onEnterPressed:  () => internal.selectRoom()

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
        placeholderText: qsTr("Search for public chat rooms...")
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
        model: ChatRoomSearchModel {
            id: chatRoomSearchModel
        }
        delegate: Item {
            id: delg

            required property int index
            required property string id
            required property string name
            required property string topic
            required property int numberOfJoinedMembers
            required property int joinRule

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

            Label {
                text: delg.name
                elide: Text.ElideRight
                anchors {
                    bottom: parent.verticalCenter
                    left: parent.left
                    right: parent.right
                    leftMargin: 20
                    rightMargin: 20
                }
            }

            Label {
                id: secondaryLabel
                color: Theme.secondaryTextColor
                elide: Text.ElideRight
                maximumLineCount: 1
                text: delg.topic
                      ? qsTr("%n member(s), topic: %1", "", delg.numberOfJoinedMembers).arg(delg.topic)
                      : qsTr("%n member(s)", "", delg.numberOfJoinedMembers)
                anchors {
                    top: parent.verticalCenter
                    left: parent.left
                    right: parent.right
                    leftMargin: 20
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
                onTapped: () => internal.selectRoom()
            }
        }
    }
}
