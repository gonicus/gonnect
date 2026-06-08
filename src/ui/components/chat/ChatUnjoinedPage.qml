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
        text: qsTr("You have been invited to join this room. Do you want to join it now?")

        Row {
            height: acceptButton.implicitHeight
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("Decline")
                highlighted: true
                icon.source: Icons.dialogCancel
                Material.accent: Theme.redColor
                onClicked: () => control.chatProvider.respondToInvitation(control.chatRoom.id, false)
            }

            Button {
                id: acceptButton
                text: qsTr("Accept")
                highlighted: true
                icon.source: Icons.objectSelectSymbolic
                Material.accent: Theme.greenColor
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
