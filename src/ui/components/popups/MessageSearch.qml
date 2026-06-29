pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 800
    implicitHeight: 600

    property var searchProvider: ChatMessageSearchProvider

    QtObject {
        id: internal

        property Item selectedItem

        readonly property Timer focusTimer: Timer {
            interval: 100
            running: true

            onTriggered: () => searchTextField.forceActiveFocus()
        }

        function selectMessage() {
            if (internal.selectedItem) {
                const roomUid = internal.selectedItem.roomUid
                const provider = control.searchProvider.getChatProviderByRoom(roomUid)
                if (provider) {
                    // TODO: This only switches to the room, without jumping to the
                    // concrete message
                    ViewHelper.showChatRoom(provider, roomUid)
                }

                control.StackView.view.popCurrentItem(StackView.Immediate)
            }
        }
    }

    Component.onDestruction: {
        control.searchProvider.searchPhrase = ""
    }

    Keys.onReturnPressed: () => internal.selectMessage()
    Keys.onEnterPressed:  () => internal.selectMessage()

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
        placeholderText: qsTr("Search for messages...")
        anchors {
            top: parent.top
            left: parent.left
            right: closeButton.left
            margins: 20
        }

        onTextEdited: () => control.searchProvider.searchPhrase = searchTextField.text.trim()
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
        model: ChatMessageSearchProvider.model
        delegate: Item {
            id: delg
            width: ListView.view.width
            height: resultColumn.implicitHeight + 10

            required property string messageUid
            required property string roomUid
            required property double rank

            property ChatMessage message: control.searchProvider.getChatMessage(roomUid, messageUid)

            Rectangle {
                id: background
                visible: internal.selectedItem === delg
                color: Theme.backgroundOffsetHoveredColor
                anchors.fill: parent
                radius: 4
            }

            Column {
                id : resultColumn
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: 10
                }
                spacing: 4

                Label {
                    id: messageText
                    width: parent.width
                    text: control.searchProvider.getChatMessageText(delg.message)
                    wrapMode: Text.WordWrap
                    //maximumLineCount: 10
                }

                Label {
                    id: messageSender
                    width: parent.width
                    text: qsTr("From: %1").arg(delg.message?.nickName ?? "")
                    color: Theme.secondaryTextColor
                    elide: Text.ElideRight
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
                onTapped: () => internal.selectMessage()
            }
        }
    }
}
