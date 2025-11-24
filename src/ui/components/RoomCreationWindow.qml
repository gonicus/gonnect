pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "roomCreationWindow"
    title: qsTr("Create a new conference")
    width: 600
    height: 340
    visible: true
    resizable: false
    showMinimizeButton: false
    showMaximizeButton: false

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    required property var dateEventDelegate

    ColumnLayout {
        id: roomCreation
        spacing: 5
        anchors {
            fill: parent
            margins: 20
        }

        Label {
            id: roomLabel
            text: qsTr("Enter a room name for the conference")
        }

        TextArea {
            id: roomName
            Layout.fillWidth: true
        }

        RowLayout {
            id: roomOptions
            spacing: 10
            Layout.fillWidth: true
            layoutDirection: Qt.RightToLeft
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom

            Button {
                id: roomCancel
                text: qsTr("Cancel")

                onPressed: control.close()
            }

            Button {
                id: roomConfirm
                icon.source: Icons.listAdd
                text: qsTr("Add")

                onPressed: {
                    if (ViewHelper.isValidJitsiRoomName(roomName.text)) {
                        dateEventDelegate.updateRoom(roomName.text)
                        ViewHelper.requestMeeting(dateEventDelegate.roomName)
                    }

                    control.close()
                }
            }
        }
    }
}
