pragma ComponentBehavior: Bound

import QtQuick
import base


Item {
    id: control
    width: 400
    height: 400

    LoggingCategory {
        id: category
        name: "gonnect.qml.EmojiPicker"
        defaultLogLevel: LoggingCategory.Warning
    }

    EmojiModel { id: emojiModel }

    signal emojiPicked(string emoji)

    function scrollToGroup(groupIndex : int) {
        for (let i = 0; i < emojiList.count; ++i) {
            const item = emojiList.itemAt(i)
            if (item.groupIndex === groupIndex) {
                emojiFlickable.contentY = Math.min(item.y, emojiFlickable.contentHeight - emojiFlickable.height)
                return
            }
        }

        console.warn(category, "unable to find rendered group item for group", groupIndex)
    }

    Flickable {
        id: tabbar
        height: 30
        clip: true
        contentWidth: groupButtonRow.implicitWidth
        anchors {
            top: parent.top
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

    Flickable {
        id: emojiFlickable
        contentHeight: emojiContainer.implicitHeight
        clip: true
        anchors {
            top: tabbar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Column {
            id: emojiContainer
            anchors {
                left: parent.left
                right: parent.right
            }

            Repeater {
                id: emojiList
                model: emojiGroupsRepeater.model

                delegate: Item {
                    id: sectionDelg
                    implicitHeight: emojiGrid.height
                    height: sectionDelg.implicitHeight
                    anchors {
                        left: parent?.left
                        right: parent?.right
                    }

                    required property int groupIndex

                    GridView {
                        id: emojiGrid
                        cellWidth: 30
                        cellHeight: emojiGrid.cellWidth
                        height: emojiGrid.contentHeight
                        interactive: false
                        anchors {
                            left: parent.left
                            right: parent.right
                        }
                        model: EmojiProxyModel {
                            sourceModel: emojiModel
                            group: sectionDelg.groupIndex
                        }
                        delegate: EmojiButton {
                            id: emojiDelg
                            emojiChar: emojiDelg.emoji
                            tooltipText: emojiDelg.label

                            required property string emoji
                            required property string label

                            onClicked: () => control.emojiPicked(emojiDelg.emoji)
                        }
                    }
                }
            }
        }
    }
}
