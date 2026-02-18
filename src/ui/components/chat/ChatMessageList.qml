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

        Accessible.role: Accessible.List
        Accessible.name: qsTr("Chat message list")
        Accessible.description: qsTr("List of all the chat messages of the current chat room")

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

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Scroll down")
        Accessible.focusable: true
        Accessible.onPressAction: () => autoScrollDownButton.tapAction()

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

            Accessible.ignored: true
        }

        HoverHandler {
            id: autoScrollHoverHandler
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onTapped: () => autoScrollDownButton.tapAction()
        }

        function tapAction() {
            internal.autoScrollBottom = true
            listView.positionViewAtEnd()
        }
    }
}
