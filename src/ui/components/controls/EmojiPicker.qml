pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base


Item {
    id: control
    width: 400
    height: 430

    LoggingCategory {
        id: category
        name: "gonnect.qml.EmojiPicker"
        defaultLogLevel: LoggingCategory.Warning
    }

    EmojiModel { id: emojiModel }

    signal emojiPicked(string emoji)

    function scrollToGroup(groupIndex : int) {
        const idx = emojiProxy.firstIndexOfGroup(groupIndex)
        if (idx >= 0) {
            emojiGrid.positionViewAtIndex(idx, GridView.Beginning)
        } else {
            console.warn(category, "no visible items for group", groupIndex)
        }
    }

    SearchField {
        id: searchField
        placeHolderText: qsTr("Search for emoji...")
        anchors {
            left: parent.left
            right: parent.right
        }

        Component.onCompleted: () => searchField.giveFocus()

        Connections {
            target: control.Window
            function onActiveChanged() {
                if (control.Window.active) {
                    searchField.giveFocus()
                }
            }
        }
    }

    Flickable {
        id: tabbar
        height: 30
        clip: true
        contentWidth: groupButtonRow.implicitWidth
        anchors {
            top: searchField.bottom
            left: parent.left
            right: parent.right
        }

        Row {
            id: groupButtonRow
            height: tabbar.height
            anchors {
                top: parent.top
                bottom: parent.bottom
            }

            Accessible.role: Accessible.Row
            Accessible.name: qsTr("Switch Emoji category")
            Accessible.focusable: true

            Repeater {
                id: emojiGroupsRepeater
                model: EmojiGroupsModel {}
                delegate: EmojiButton {
                    id: groupDelegate
                    emojiChar: groupDelegate.emoji
                    anchors {
                        top: parent.top
                        bottom: parent.bottom
                    }

                    required property int groupIndex
                    required property string emoji

                    onClicked: () => control.scrollToGroup(groupDelegate.groupIndex)
                }
            }
        }
    }

    Rectangle {
        color: Theme.borderColor
        height: 1
        z: 10
        anchors {
            left: parent.left
            right: parent.right
            top: tabbar.bottom
        }
    }

    GridView {
        id: emojiGrid
        cellWidth: 30
        cellHeight: emojiGrid.cellWidth
        interactive: true
        clip: true
        anchors {
            top: tabbar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Accessible.role: Accessible.Column
        Accessible.name: qsTr("Select Emoji")
        Accessible.focusable: true

        model: EmojiProxyModel {
            id: emojiProxy
            sourceModel: emojiModel
            filterText: searchField.text.trim()
        }

        delegate: EmojiButton {
            id: emojiDelg
            emojiChar: emojiDelg.emoji
            tooltipText: emojiDelg.label

            required property string emoji
            required property string label

            onClicked: () => control.emojiPicked(emojiDelg.emoji)
        }

        ScrollBar.vertical: ScrollBar { width: 5 }
    }
}
