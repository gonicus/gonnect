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

    property ChatMessageContentFile content

    property int fileSize

    IconLabel {
        id: fileLabel
        spacing: 10
        text: (control.content?.fileSize ?? 0) > 0
              ? `${control.content?.fileName || qsTr("File")} (${TextFormatHelper.formatFileSize(control.content?.fileSize ?? 0)})`
              : (control.content?.fileName
                 ? control.content.fileName
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
        selectedFile: `file://${FileHelper.downloadFolderPath()}/${control.control?.fileName ?? ""}`
        onAccepted: () => FileHelper.copyFile(control.content?.filePath ?? "", saveFileDialog.selectedFile)
    }
}
