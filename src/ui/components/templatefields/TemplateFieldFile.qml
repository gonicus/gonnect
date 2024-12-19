import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs

Item {
    id: control
    implicitWidth: chooseButton.x + chooseButton.implicitWidth
    implicitHeight: textField.implicitHeight

    readonly property alias value: textField.text

    property var regex
    property list<string> fileSuffixes

    TextField {
        id: textField
        anchors {
            left: parent.left
            right: chooseButton.left
            rightMargin: 20
        }
    }

    Button {
        id: chooseButton
        text: qsTr("Choose...")
        anchors {
            verticalCenter: textField.verticalCenter
            right: parent.right
        }
        onClicked: () => fileDialog.open()
    }

    FileDialog {
        id: fileDialog
        nameFilters: [ qsTr("Certificate files (%1)").arg(control.fileSuffixes.map(suffix => `*.${suffix}`).join(" ")) ]
        onAccepted: () => textField.text = fileDialog.selectedFile
    }
}
