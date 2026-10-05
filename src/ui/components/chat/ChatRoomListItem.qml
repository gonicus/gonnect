pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Item {
    id: control
    height: control.implicitHeight
    implicitHeight: content.y + content.height + (threadCol.visible ? threadCol.implicitHeight : 0)

    property IChatRoom chatRoom: null

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
    signal filesDropped(list<url> urls)
    signal threadSelected(string threadId)

    ChatRoomListSectionHeader {
        id: sectionHeaderItem
        visible: !!sectionHeaderItem.text
        text: control.sectionHeader
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            leftMargin: Theme.d
            rightMargin: Theme.d
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
                leftMargin: Theme.d
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
                leftMargin: Theme.d
                rightMargin: Theme.d
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
            exclusiveSignals: TapHandler.SingleTap
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onTapped: (_, mouseButton) => {
                if (mouseButton === Qt.RightButton) {
                    contextMenuComponent.createObject(control, {
                                                          editRoomVisible: !!(Number(control.permissions ?? 0) & IChatRoom.Permission.CanEdit),
                                                          inviteUsersVisible: !!(Number(control.permissions ?? 0) & IChatRoom.Permission.CanInvite)
                                                      }).popup()
                } else {
                    control.clicked()
                }
            }
        }
    }

    Column {
        id: threadCol
        visible: control.highlighted && threadRepeater.count > 0
        anchors {
            left: content.left
            right: content.right
            top: content.bottom
            leftMargin: 2 * Theme.d
        }

        Repeater {
            id: threadRepeater
            model: ChatThreadModel {
                chatRoom: control.chatRoom
            }
            delegate: ChatRoomListThreadItem {
                id: threadDelg
                highlighted: SelectionState.selectedThreadId === threadDelg.threadId
                anchors {
                    left: parent?.left
                    right: parent?.right
                }

                onClicked: () => control.threadSelected(threadDelg.highlighted ? "" : threadDelg.threadId)
            }
        }
    }

    FileDropArea {
        anchors.fill: content
        compact: true
        onDropAccepted: urls => control.filesDropped(urls)
    }

    Component {
        id: contextMenuComponent

        ChatRoomContextMenu {
            id: contextMenu

            onFavoriteToggled: () => control.favoriteToggled()
            onLeaveRoomTriggered: () => control.leaveRoomTriggered()
            onEditRoomTriggered: () => control.editRoomTriggered()
            onInviteUsersTriggered: () => control.inviteUsersTriggered()
        }
    }
}
