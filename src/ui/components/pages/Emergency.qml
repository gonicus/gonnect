pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 800
    implicitHeight: 500

    function close() {
        if (control.StackView.view) {
            control.StackView.view.popCurrentItem(StackView.Immediate)
        } else {
            SelectionState.selectedPage = {
                id: SelectionState.homePageId(),
                type: MainPageSelection.PageType.Base,
                attachedData: null
            }
        }
    }

    Flickable {
        anchors.fill: parent
        clip: true
        flickableDirection: Flickable.AutoFlickIfNeeded
        contentHeight: options.implicitHeight
        ScrollBar.vertical: ScrollBar { width: 10 }

        Accessible.role: Accessible.ButtonMenu
        Accessible.name: firstAidHeader.text
        Accessible.description: firstAidDescription.text

        Shortcut {
            sequence: "Esc"
            enabled: control.visible
            onActivated: control.close()
        }

        ColumnLayout {
            id: options
            spacing: 20
            anchors.fill: parent

            Label {
                id: firstAidHeader
                text: qsTr("Emergency Call")
                font.pixelSize: 32
                wrapMode: Label.Wrap

                Accessible.ignored: true
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
                        control.close()
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

            Item {
                Layout.preferredHeight: 20
            }

            Button {
                id: firstAidExit
                text: qsTr("Close")
                Layout.preferredWidth: control.implicitWidth / 2
                Layout.alignment: Qt.AlignHCenter

                onClicked: control.close()

                Accessible.role: Accessible.Button
                Accessible.name: firstAidExit.text
                Accessible.description: qsTr("Exit the emergency call menu without initiating any action")
                Accessible.focusable: true
                Accessible.onPressAction: () => firstAidExit.click()
            }
        }
    }

    HeaderIconButton {
        id: closeButton
        iconSource: Icons.mobileCloseApp
        accessiblePurpose: qsTr("Close emergency call menu")
        anchors {
            top: parent.top
            right: parent.right
        }

        onClicked: () => control.close()
    }
}
