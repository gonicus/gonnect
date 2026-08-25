pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property IChatProvider chatProvider
    property IChatRoom chatRoom
    property int joinState

    component JoinContainer : Item {
        anchors.fill: parent

        default property alias content: innerCol.children
        property alias text: mainLabel.text

        Column {
            id: innerCol
            spacing: 40
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: parent.right
            }

            Label {
                id: mainLabel
                wrapMode: Text.Wrap
                horizontalAlignment: Label.AlignHCenter
                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: 20
                    rightMargin: 20
                }
            }
        }
    }

    JoinContainer {
        visible: !control.chatRoom
        text: qsTr("Select a room in the list or via search to open it.")
    }

    JoinContainer {
        visible: control.chatRoom && control.joinState === IChatRoom.UserRoomState.Unjoined
        text: qsTr("You are currently not a member of this room.")

        Button {
            text: qsTr("Join")
            visible: control.chatRoom?.joinRule === IChatRoom.JoinRule.Public ?? false
            highlighted: true
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: () => control.chatProvider.joinRoomRequest(control.chatRoom.id)
        }

        Label {
            text: qsTr("Optional message for the user that receives the knock:")
            visible: knockButton.visible
            anchors.horizontalCenter: parent.horizontalCenter
        }

        TextArea {
            visible: knockButton.visible
            width: 0.64 * parent.width
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Button {
            id: knockButton
            text: qsTr("Knock")
            visible: control.chatRoom?.joinRule === IChatRoom.JoinRule.Knock ?? false
            highlighted: true
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: () => control.chatProvider.knockRoomRequest(control.chatRoom.id)
        }

        Label {
            text: qsTr("You can only join this room by being invited.")
            visible: control.chatRoom?.joinRule === IChatRoom.JoinRule.Invite ?? false
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    JoinContainer {
        visible: control.joinState === IChatRoom.UserRoomState.Invited
        text: qsTr("You have been invited to join this room '%1'.").arg(control.chatRoom?.name ?? "")

        Label {
            text: qsTr("Invitation message:")
            visible: invitationTextLabel.text !== ""
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            id: invitationTextLabelContainer
            visible: invitationTextLabel.text !== ""
            height: invitationTextLabel.height + 2 * 20
            radius: 8
            color: Theme.backgroundSecondaryColor
            anchors {
                left: parent.left
                right: parent.right
            }

            Label {
                id: invitationTextLabel
                text: control.chatRoom?.invitationText?.trim() ?? ""
                wrapMode: Text.Wrap
                font.italic: true
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    margins: 20
                }
            }
        }

        Label {
            text: qsTr("Do you want to join this chat room?")
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Row {
            height: acceptButton.implicitHeight
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("Decline")
                highlighted: true
                icon.source: Icons.dialogCancel
                Material.accent: Theme.redColor

                Accessible.role: Accessible.Button
                Accessible.name: text

                onClicked: () => control.chatProvider.respondToInvitation(control.chatRoom.id, false)
            }

            Button {
                id: acceptButton
                text: qsTr("Join")
                highlighted: true
                icon.source: Icons.objectSelectSymbolic
                Material.accent: Theme.greenColor

                Accessible.role: Accessible.Button
                Accessible.name: text

                onClicked: () => control.chatProvider.respondToInvitation(control.chatRoom.id, true)
            }
        }
    }

    JoinContainer {
        visible: control.joinState === IChatRoom.UserRoomState.Knocked
        text: qsTr("You knocked on the door and are waiting for someone to let you in.")
    }

    JoinContainer {
        visible: control.joinState === IChatRoom.UserRoomState.Banned
        text: qsTr("You have been banned from this room and may not enter it again, unless a room administrator re-invites you.")
    }
}
