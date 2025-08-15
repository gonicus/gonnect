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

            Repeater {
                model: EmergencyContactsModel {}
                delegate: Button {
                    id: delg
                    text: delg.displayName
                    anchors.horizontalCenter: parent.horizontalCenter
                    highlighted: true
                    Material.accent: Theme.redColor

                    onClicked: () => {
                        SIPCallManager.endAllCalls()
                        SIPCallManager.call(delg.number)
                        control.StackView.view.popCurrentItem(StackView.Immediate)
                    }

                    required property string number
                    required property string displayName
                }
            }
        }
    }

    HeaderIconButton {
        id: closeButton
        iconSource: Icons.mobileCloseApp
        anchors {
            top: parent.top
            right: parent.right
        }

        onClicked: () => control.StackView.view.popCurrentItem(StackView.Immediate)
    }
}
