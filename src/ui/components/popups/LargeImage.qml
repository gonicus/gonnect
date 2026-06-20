pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: imageItem.sourceSize.width
    implicitHeight: imageItem.sourceSize.height

    property alias source: imageItem.source

    Item {
        id: imageContainer
        anchors {
            top: parent.top
            bottom: buttonRow.top
            left: parent.left
            right: parent.right
            margins: 20
        }

        AnimatedImage {
            id: imageItem
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            width: Math.min(imageContainer.width, imageItem.sourceSize.width)
            height: Math.min(imageContainer.height, imageItem.sourceSize.height)

            TapHandler {
                grabPermissions: PointerHandler.TakeOverForbidden
                gesturePolicy: TapHandler.WithinBounds
                onSingleTapped: () => {
                    control.StackView.view.popCurrentItem(StackView.Immediate)
                }
            }
        }
    }

    Row {
        id: buttonRow
        spacing: 20
        anchors {
            bottom: parent.bottom
            bottomMargin: 20
            horizontalCenter: parent.horizontalCenter
        }

        Button {
            id: copyToClipboardButton
            icon.source: Icons.editCopy
            text: qsTr("Copy to clipboard")
            onClicked: () => ClipboardHelper.copyImageToClipboard(control.source)
        }

        Button {
            id: saveButton
            highlighted: true
            icon.source: Icons.documentSave
            text: qsTr("Save")
            onClicked: () => saveFileDialog.open()
        }
    }

    FileDialog {
        id: saveFileDialog
        fileMode: FileDialog.SaveFile
        currentFolder: `file://${FileHelper.downloadFolderPath()}`
        selectedFile: {
            let suffix = ""
            const splitted = control.source.toString().split(".")
            if (splitted.length) {
                suffix = splitted[splitted.length - 1]
            }
            const fileName = `${qsTr("Untitled")}.${suffix}`

            return `file://${FileHelper.downloadFolderPath()}/${fileName}`
        }
        onAccepted: () => FileHelper.copyFile(control.source, saveFileDialog.selectedFile)
    }
}
