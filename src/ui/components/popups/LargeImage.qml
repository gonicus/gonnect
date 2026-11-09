pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: Math.max(imageLoader.originSize.width + 2 * Theme.d, buttonRow.implicitWidth + 2 * Theme.d)
    implicitHeight: imageLoader.originSize.height + buttonRow.implicitHeight + 3 * Theme.d

    property url source

    Item {
        id: imageContainer
        anchors {
            top: parent.top
            bottom: buttonRow.top
            left: parent.left
            right: parent.right
            margins: 20
        }

        Loader {
            id: imageLoader
            sourceComponent: FileHelper.isAnimatedImage(control.source) ? animatedImageComponent : imageComponent
            anchors.centerIn: parent

            readonly property size originSize: imageLoader.item?.sourceSize ?? Qt.size(0, 0)
        }

        TapHandler {
            grabPermissions: PointerHandler.TakeOverForbidden
            gesturePolicy: TapHandler.WithinBounds
            onSingleTapped: () => {
                control.StackView.view.popCurrentItem(StackView.Immediate)
            }
        }

        Component {
            id: imageComponent

            Image {
                id: imageItem
                autoTransform: true
                source: control.source
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
                width: Math.min(imageContainer.width, imageItem.sourceSize.width)
                height: Math.min(imageContainer.height, imageItem.sourceSize.height)
            }
        }

        Component {
            id: animatedImageComponent

            AnimatedImage {
                id: animatedImageItem
                autoTransform: true
                source: control.source
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
                width: Math.min(imageContainer.width, animatedImageItem.sourceSize.width)
                height: Math.min(imageContainer.height, animatedImageItem.sourceSize.height)
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
            onClicked: () => {
                           ClipboardHelper.copyImageToClipboard(control.source)
                           copyToClipboardButton.icon.source = Icons.checkbox
                           feedbackTimer.start()
                       }

            Timer {
                id: feedbackTimer
                interval: Theme.feedbackTimeout
                repeat: false
                onTriggered: () => copyToClipboardButton.icon.source = Icons.editCopy
            }
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
