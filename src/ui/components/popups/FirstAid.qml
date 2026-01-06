pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
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
        contentHeight: options.implicitHeight
        ScrollBar.vertical: ScrollBar { width: 10 }

        Shortcut {
            sequence: "Esc"
            onActivated: control.StackView.view.popCurrentItem(StackView.Immediate)
        }

        ColumnLayout {
            id: options
            spacing: 20
            anchors.fill: parent

            Label {
                id: firstAidHeader
                text: qsTr("First Aid")
                font.pixelSize: 32
                wrapMode: Label.Wrap
            }

            Label {
                id: firstAidDescription
                text: qsTr("Clicking one of these buttons will end all current calls and start an emergency call.")
                wrapMode: Label.Wrap
            }

            Repeater {
                model: EmergencyContactsModel {}
                delegate: Button {
                    id: delg
                    text: delg.displayName
                    highlighted: true
                    Material.accent: Theme.redColor
                    Layout.preferredWidth: control.implicitWidth / 2
                    Layout.alignment: Qt.AlignHCenter

                    onClicked: () => {
                        SIPCallManager.endAllCalls()
                        SIPCallManager.call(delg.number)
                        control.StackView.view.popCurrentItem(StackView.Immediate)
                    }

                    required property string number
                    required property string displayName
                }
            }

            Item {
                Layout.preferredHeight: 20
            }

            Button {
                id: firstAidExit
                text: qsTr("Cancel")
                Layout.preferredWidth: control.implicitWidth / 2
                Layout.alignment: Qt.AlignHCenter

                onClicked: {
                    control.StackView.view.popCurrentItem(StackView.Immediate)
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
