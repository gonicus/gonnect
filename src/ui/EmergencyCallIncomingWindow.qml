pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "emergencyCallIncomingWindow"
    width: 800
    height: 400
    visible: true
    resizable: false
    title: qsTr("Emergency Call")

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    required property string accountId
    required property int callId
    required property string displayName

    Item {
        id: container
        anchors.fill: parent

        Item {
            id: cross
            width: 100
            height: cross.width
            anchors {
                left: parent.left
                leftMargin: 40
                verticalCenter: parent.verticalCenter
            }

            Rectangle {
                color: Theme.redColor
                width: cross.width / 4
                radius: height / 2
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    horizontalCenter: parent.horizontalCenter
                }
            }

            Rectangle {
                color: Theme.redColor
                height: cross.height / 4
                radius: width / 2
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
            }
        }

        Item {
            anchors {
                left: cross.right
                right: parent.right
                leftMargin: 40
                rightMargin: 40
                top: parent.top
                bottom: answerCallButton.top
            }

            Column {
                id: labelColumn
                spacing: 20
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    text: control.displayName
                    wrapMode: Label.Wrap
                    font.pixelSize: 36
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                Label {
                    text: qsTr("Answering the call will automatically terminate all other ongoing calls.")
                    wrapMode: Label.Wrap
                    font.pixelSize: 14
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
            }
        }

        Button {
            id: answerCallButton
            text: qsTr("Answer")
            Material.accent: Theme.redColor
            highlighted: true
            icon.source: Icons.callStart
            anchors {
                bottom: parent.bottom
                bottomMargin: 20
                horizontalCenter: parent.horizontalCenter
            }

            onClicked: () => {
                SIPCallManager.acceptCall(control.accountId, control.callId)
                control.close()
                control.destroy()
            }
        }
    }
}
