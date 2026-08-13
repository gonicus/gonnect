pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 360
    implicitHeight: container.implicitHeight

    property var numbers
    property string code

    property IChatProvider chatProvider
    property string roomId
    property string roomDisplayName
    property string invitationText

    QtObject {
        id: internal

        function answer(accept : bool) {
            control.chatProvider.respondToInvitation(control.roomId, accept)
            ViewHelper.topDrawer.loader.sourceComponent = undefined
        }
    }

    Column {
        id: container
        topPadding: 20
        spacing: 20
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        Label {
            text: qsTr('You have been invited to the chat room "%1".').arg(control.roomDisplayName)
            wrapMode: Text.Wrap
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Label {
            text: qsTr("Invitation message:")
            wrapMode: Text.Wrap
            visible: invitationTextLabelContainer.visible
            anchors {
                left: parent.left
                right: parent.right
            }
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
                text: control.invitationText.trim()
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
            wrapMode: Text.Wrap
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Row {
            height: acceptButton.implicitHeight
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Button {
                text: qsTr("Decline")
                icon.source: Icons.dialogCancel
                highlighted: true
                Material.accent: Theme.redColor

                onClicked: () => internal.answer(false)
            }

            Button {
                id: acceptButton
                text: qsTr("Accept")
                icon.source: Icons.objectSelectSymbolic
                highlighted: true
                Material.accent: Theme.greenColor

                onClicked: () => internal.answer(true)
            }
        }
    }
}
