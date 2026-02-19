import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs

Item {
    id: control
    implicitWidth: chooseButton.x + chooseButton.implicitWidth
    implicitHeight: textField.implicitHeight

    property string templateFieldName

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

        Accessible.role: Accessible.EditableText
        Accessible.name: qsTr("File path")
        Accessible.description: qsTr("Enter the file path for the ") + control.templateFieldName
        Accessible.focusable: true
    }

    Button {
        id: chooseButton
        text: qsTr("Choose...")
        anchors {
            verticalCenter: textField.verticalCenter
            right: parent.right
        }
        onClicked: () => fileDialog.open()

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Open file picker")
        Accessible.description: qsTr("Select the file that should be used for the ") + control.templateFieldName
        Accessible.focusable: true
        Accessible.onPressAction: chooseButton.click()
    }

    FileDialog {
        id: fileDialog
        nameFilters: [ qsTr("Certificate files (%1)").arg(control.fileSuffixes.map(suffix => `*.${suffix}`).join(" ")) ]
        onAccepted: () => textField.text = fileDialog.selectedFile
    }
}
