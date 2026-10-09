pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

Item {
    id: control
    height: 3.5 * Theme.d

    required property string threadId
    required property ChatMessage firstMessage
    required property ChatMessage lastMessage
    required property int unreadCount

    property alias highlighted: selectedBackground.visible

    signal clicked

    Rectangle {
        id: selectedBackground
        visible: false
        color: Theme.backgroundOffsetHoveredColor
        radius: 4
        anchors.fill: parent

        Accessible.ignored: true
    }

    Rectangle {
        id: hoverBackground
        visible: hoverHandler.hovered
        color: Theme.backgroundOffsetHoveredColor
        radius: 4
        anchors.fill: parent
    }

    IconLabel {
        id: threadIcon
        width: Theme.d * 2
        height: Theme.d * 2
        icon {
            width: Theme.d * 2 * Screen.devicePixelRatio
            height: Theme.d * 2 * Screen.devicePixelRatio
            source: Icons.messageThread
        }
        anchors {
            left: parent.left
            leftMargin: Theme.d
            verticalCenter: parent.verticalCenter
        }

        Accessible.ignored: true
    }

    Item {
        id: labelContainer
        anchors {
            left: threadIcon.right
            right: unreadBubble.visible
                   ? unreadBubble.left
                   : parent.right
            verticalCenter: parent.verticalCenter
            leftMargin: Theme.d
            rightMargin: Theme.d
        }

        Label {
            id: titleLabel
            tooltipsEnabled: false
            font.weight: control.highlighted ? Font.Medium : Font.Normal
            elide: Label.ElideRight
            text: control.lastMessage?.content?.rawText ?? ""
            maximumLineCount: 1
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
        }
    }

    Rectangle {
        id: unreadBubble
        visible: control.unreadCount > 0
        width: 2 * Theme.d
        height: unreadBubble.width
        radius: unreadBubble.width / 2
        color: Theme.redColor
        anchors {
            right: parent.right
            rightMargin: Theme.d
            verticalCenter: parent.verticalCenter
        }

        Label {
            anchors.centerIn: parent
            color: Theme.whiteColor
            font.pixelSize: Theme.fontSizeNormal
            font.weight: Font.Medium
            text: control.unreadCount > 0
                  ? (control.unreadCount > 9
                     ? ">9"
                     : control.unreadCount)
                  : "1"
        }

        Accessible.ignored: true
    }

    HoverHandler {
        id: hoverHandler
    }

    TapHandler {
        gesturePolicy: TapHandler.WithinBounds
        grabPermissions: PointerHandler.ApprovesTakeOverByAnything
        onTapped: event => {
                      event.accepted = true
                      control.clicked()
                  }
    }
}
