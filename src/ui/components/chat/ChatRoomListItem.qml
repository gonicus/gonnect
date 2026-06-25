pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Item {
    id: control
    implicitHeight: content.y + content.height

    required property int index
    required property string roomId
    required property string name
    required property string avatarPath
    required property int unreadCount
    required property bool isFavorite
    required property bool hasPresenceState
    required property int presenceState
    required property int permissions
    required property int ownJoinState

    required property string sectionHeader

    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("Chat room")
    Accessible.description: qsTr("Selected chat room %1: %2 unread messages").arg(control.name).arg(control.unreadCount)
    Accessible.focusable: true
    Accessible.onPressAction: () => control.clicked()

    property alias highlighted: selectedBackground.visible

    signal clicked
    signal favoriteToggled
    signal leaveRoomTriggered
    signal editRoomTriggered
    signal inviteUsersTriggered
    signal fileDropped(string url)

    ChatRoomListSectionHeader {
        id: sectionHeaderItem
        visible: !!sectionHeaderItem.text
        text: control.sectionHeader
        anchors {
            top: parent.top
            topMargin: 20
            left: parent.left
            right: parent.right
            leftMargin: 10
            rightMargin: 10
        }
    }

    Item {
        id: content
        height: 54
        anchors {
            top: sectionHeaderItem.visible ? sectionHeaderItem.bottom : parent.top
            right: parent.right
            left: parent.left
        }

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

        AvatarImage {
            id: avatarImage
            size: 40
            source: control.avatarPath
            initials: ViewHelper.initials(delg.name)
            showPresenceStatus: control.hasPresenceState
            presenceStatus: control.presenceState
            indicatorComponent: Component { ChatUserPresenceStatusIndicator {} }
            anchors {
                left: parent.left
                leftMargin: 10
                verticalCenter: parent.verticalCenter
            }

            Accessible.ignored: true
        }

        Item {
            id: labelContainer
            anchors {
                left: avatarImage.right
                right: unreadBubble.visible
                       ? unreadBubble.left
                       : parent.right
                verticalCenter: parent.verticalCenter
                leftMargin: 10
                rightMargin: 10
            }

            Label {
                id: nameLabel
                text: control.name
                font.weight: control.highlighted ? Font.Medium : Font.Normal
                elide: Label.ElideRight
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
            }
        }

        Rectangle {
            id: unreadBubble
            visible: control.unreadCount > 0 || control.ownJoinState !== IChatRoom.UserRoomState.Joined
            width: 24
            height: unreadBubble.width
            radius: unreadBubble.width / 2
            color: Theme.redColor
            anchors {
                right: parent.right
                rightMargin: 10
                verticalCenter: parent.verticalCenter
            }

            Label {
                anchors.centerIn: parent
                color: Theme.foregroundWhiteColor
                font.pixelSize: 14
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
            exclusiveSignals: TapHandler.SingleTap
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onTapped: (_, mouseButton) => {
                if (mouseButton === Qt.RightButton) {
                    contextMenuComponent.createObject(control).popup()
                } else {
                    control.clicked()
                }
            }
        }
    }

    FileDropArea {
        anchors.fill: content
        compact: true
        onDropAccepted: url => control.fileDropped(url)
    }

    Component {
        id: contextMenuComponent

        Menu {
            id: contextMenu
            onClosed: () => contextMenu.destroy()

            Action {
                text: qsTr("Toggle favorite")
                icon.source: Icons.folderFavorites
                onTriggered: () => control.favoriteToggled()
            }

            Action {
                text: qsTr("Leave room...")
                icon.source: Icons.dialogCancel
                onTriggered: () => control.leaveRoomTriggered()
            }

            Action {
                text: qsTr("Edit room...")
                icon.source: Icons.editor
                enabled: !!(control.permissions & IChatRoom.Permission.CanEdit)
                onTriggered: () => control.editRoomTriggered()
            }

            Action {
                text: qsTr("Invite users...")
                icon.source: Icons.listAdd
                enabled: !!(control.permissions & IChatRoom.Permission.CanInvite)
                onTriggered: () => control.inviteUsersTriggered()
            }
        }
    }
}
