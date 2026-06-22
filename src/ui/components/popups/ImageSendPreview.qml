pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 400
    implicitHeight: col.implicitHeight

    property alias source: imageItem.source
    property IChatRoom chatRoom

    function close() {
        ViewHelper.topDrawer.loader.sourceComponent = undefined
    }

    Keys.onEnterPressed: () => sendButton.clicked()
    Keys.onReturnPressed: () => sendButton.clicked()
    Keys.onEscapePressed: () => control.close()

    Column {
        id: col
        spacing: 20
        topPadding: 20
        anchors {
            left: parent.left
            right: parent.right
            leftMargin: 10
            rightMargin: 10
        }

        AnimatedImage {
            id: imageItem
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignHCenter
            verticalAlignment: Image.AlignVCenter
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Label {
            text: {
                let roomName = ""
                const room = control.chatRoom
                if (room) {
                    if (room.name) {
                        roomName = room.name
                    } else {
                        roomName = room.id
                    }
                }

                return qsTr("Do you want to send this image in chat room '%1'?").arg(roomName)
            }
            wrapMode: Label.Wrap
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Row {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("Cancel")
                onClicked: () => control.close()
            }

            Button {
                id: sendButton
                highlighted: true
                text: qsTr("Send")
                onClicked: () => {
                    control.chatRoom.sendFile(control.source)
                    control.close()
                }
            }
        }
    }
}
