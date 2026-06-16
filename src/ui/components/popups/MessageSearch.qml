pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 800
    implicitHeight: 600

    // TODO: The singleton instance should likely be started ASAP
    // to get some intitial message loading done prior to the first search
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
                // TODO: Go to chat room that contains message item

                control.StackView.view.popCurrentItem(StackView.Immediate)
            }
        }
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

            required property string messageUid
            required property double rank

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

            // TODO: Map messageUid to message text
            Label {
                text: delg.messageUid
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
                text: delg.rank
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
                onTapped: () => internal.selectMessage()
            }
        }
    }
}
