pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Card {
    id: control
    anchors {
        fill: parent
        leftMargin: Theme.d * 2
        rightMargin: Theme.d * 2
        topMargin: Theme.d
        bottomMargin: Theme.d
    }

    Flickable {
        id: flickable
        clip: true
        flickableDirection: Flickable.AutoFlickIfNeeded
        contentHeight: options.implicitHeight
        height: Math.min(control.height, flickable.contentHeight)
        ScrollBar.vertical: ScrollBar { width: 10 }
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        Accessible.role: Accessible.ButtonMenu
        Accessible.name: firstAidHeader.text
        Accessible.description: firstAidDescription.text

        Column {
            id: options
            spacing: 20
            anchors {
                left: parent.left
                right: parent.right
            }

            Label {
                id: firstAidHeader
                text: qsTr("Emergency call")
                font.pixelSize: 32
                wrapMode: Label.Wrap
                horizontalAlignment: Label.AlignHCenter
                anchors {
                    left: parent.left
                    right: parent.right
                }
                Accessible.ignored: true
            }

            Label {
                id: firstAidDescription
                text: qsTr("Clicking one of these buttons will end all current calls and start an emergency call.")
                wrapMode: Label.Wrap
                horizontalAlignment: Label.AlignHCenter
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Accessible.ignored: true
            }

            Repeater {
                model: EmergencyContactsModel {}
                delegate: Button {
                    id: delg
                    text: delg.displayName
                    highlighted: true
                    Material.accent: Theme.redColor
                    anchors.horizontalCenter: parent.horizontalCenter

                    onClicked: () => {
                        SIPCallManager.endAllCalls()
                        SIPCallManager.call(delg.number)
                    }

                    required property string number
                    required property string displayName

                    Accessible.role: Accessible.Button
                    Accessible.name: delg.displayName
                    Accessible.description: qsTr("Tap to call emergency contact: %1 (%2)").arg(delg.displayName).arg(delg.number)
                    Accessible.focusable: true
                    Accessible.onPressAction: () => delg.click()
                }
            }
        }
    }
}
