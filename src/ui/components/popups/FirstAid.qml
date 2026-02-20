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

                Accessible.role: Accessible.StaticText
                Accessible.name: firstAidHeader.text
                Accessible.description: firstAidDescription.text
            }

            Label {
                id: firstAidDescription
                text: qsTr("Clicking one of these buttons will end all current calls and start an emergency call.")
                wrapMode: Label.Wrap

                Accessible.ignored: true
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
                text: qsTr("Close")
                Layout.preferredWidth: control.implicitWidth / 2
                Layout.alignment: Qt.AlignHCenter

                onClicked: {
                    control.StackView.view.popCurrentItem(StackView.Immediate)
                }

                Accessible.role: Accessible.Button
                Accessible.name: firstAidExit.text
                Accessible.description: qsTr("Exit the first aid menu without initiating any action")
                Accessible.focusable: true
                Accessible.onPressAction: () => firstAidExit.click()
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
