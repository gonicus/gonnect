pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

Item {
    id: control
    clip: true
    implicitWidth: pinnedListView.contenctWidth
    implicitHeight: pinnedListView.contentHeight

    property alias chatRoom: pinnedModel.chatRoom
    readonly property alias count: pinnedListView.count

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundOffsetColor
        border {
            width: 1
            color: Theme.borderColor
        }
    }

    ListView {
        id: pinnedListView
        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        model: PinnedChatMessages {
            id: pinnedModel
        }
        delegate: Item {
            id: delg
            implicitHeight: contentItem.height + delg.padding * 2
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property int index
            required property QtObject content
            readonly property int padding: 2 * Theme.d

            Rectangle {
                id: bottomBorder
                height: 1
                color: Theme.borderColor
                visible: delg.index !== pinnedListView.count - 1
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
            }

            IconLabel {
                id: pinIcon
                anchors {
                    left: parent.left
                    leftMargin: Theme.d * 2
                    verticalCenter: parent.verticalCenter
                }

                icon {
                    source: Icons.windowPin
                    width: Theme.d * 2
                    height: Theme.d * 2
                }
            }

            ChatMessageListItemContent {
                id: contentItem
                content: delg.content
                isStateUpdate: false
                userState: 0
                affectedUserName: ""
                anchors {
                    left: pinIcon.right
                    right: parent.right
                    top: parent.top
                    margins: delg.padding
                }
            }
        }
    }
}
