pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    property alias chatRoom: chatModel.chatRoom

    QtObject {
        id: internal

        property bool autoScrollBottom: true
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: ChatProxyModel {
            ChatModel {
                id: chatModel
            }
        }
        delegate: ChatMessageListItem {
            anchors {
                left: parent?.left
                right: parent?.right
            }
        }

        Component.onCompleted: () => listView.positionViewAtEnd()
        onCountChanged: () => {
            if (internal.autoScrollBottom) {
                listView.positionViewAtEnd()
            }
        }

        onMovementStarted: () => {
            internal.autoScrollBottom = false
        }

        onMovementEnded: () => {
            if (listView.atYEnd) {
                internal.autoScrollBottom = true
            }
        }
    }

    Item {
        id: autoScrollDownButton
        visible: !internal.autoScrollBottom
        width: 50
        height: autoScrollDownButton.width
        anchors {
            bottom: parent.bottom
            right: parent.right
        }

        Rectangle {
            id: autoScrollBackground
            anchors.fill: parent
            radius: parent.width / 2
            color: autoScrollHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor
        }

        Label {
            text: "↓"
            anchors.centerIn: parent
            font.pixelSize: 20
        }

        HoverHandler {
            id: autoScrollHoverHandler
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onTapped: () => {
                internal.autoScrollBottom = true
                listView.positionViewAtEnd()
            }
        }
    }
}
