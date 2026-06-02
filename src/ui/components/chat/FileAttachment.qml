pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import base

ChatMessageAttachmentRectangle {
    id: control
    height: 50
    width: fileLabel.implicitWidth + 2 * fileLabel.anchors.leftMargin
    color: hoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundSecondaryColor

    property string fileName
    property string fileUrl
    property int fileSize

    IconLabel {
        id: fileLabel
        spacing: 10
        text: control.fileSize > 0
              ? `${control.fileName || qsTr("File")} (${TextFormatHelper.formatFileSize(control.fileSize)})`
              : (control.fileName
                 ? control.fileName
                 : qsTr("File"))
        icon.source: Icons.mailAttachment
        font {
            pixelSize: 14
            weight: Font.DemiBold
        }
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 20
        }
    }

    HoverHandler {
        id: hoverHandler
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onSingleTapped: () => saveFileDialog.open()
    }

    FileDialog {
        id: saveFileDialog
        fileMode: FileDialog.SaveFile
        currentFolder: `file://${FileHelper.downloadFolderPath()}`
        selectedFile: `file://${FileHelper.downloadFolderPath()}/${control.fileName}`
        onAccepted: () => FileHelper.copyFile(control.fileUrl, saveFileDialog.selectedFile)
    }
}
