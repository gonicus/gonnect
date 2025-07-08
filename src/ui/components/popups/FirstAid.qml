pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 800
    implicitHeight: 500

    Flickable {
        anchors.fill: parent
        clip: true
        flickableDirection: Flickable.AutoFlickIfNeeded
        contentHeight: container.implicitHeight

        ScrollBar.vertical: ScrollBar { width: 10 }

        Column {
            id: container
            spacing: 20
            anchors {
                left: parent.left
                right: parent.right
            }

            Label {
                text: qsTr("First Aid")
                font.pixelSize: 32
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Label {
                text: qsTr("Clicking one of these buttons will end all current calls and start an emergency call.")
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Button {
                text: qsTr("Fire and rescue service")
                anchors.horizontalCenter: parent.horizontalCenter
                highlighted: true
                Material.accent: Theme.redColor
                onClicked: () => console.log('TODO: Call 112')
            }

            Button {
                text: qsTr("Police")
                anchors.horizontalCenter: parent.horizontalCenter
                highlighted: true
                Material.accent: Theme.redColor
                onClicked: () => console.log('TODO: Call 112')
            }

            Button {
                text: qsTr("First responder")
                anchors.horizontalCenter: parent.horizontalCenter
                highlighted: true
                Material.accent: Theme.redColor
                onClicked: () => console.log('TODO: Call 911')
            }
        }
    }
}
