pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import base

Rectangle {
    id: control
    implicitHeight: 48
    height: control.implicitHeight
    implicitWidth: toneName.implicitWidth
                   + testPlayButton.implicitWidth
                   + resetToDefaultButton.implicitWidth
                   + pickToneButton.implicitWidth
    width: control.implicitWidth
    radius: 4
    color: 'transparent'
    border.width: 1
    border.color: Theme.borderColor

    Accessible.role: Accessible.StaticText
    Accessible.name: toneName.text

    property string filePath
    property string placeholderText: qsTr('Default')
    property bool isPlaying
    property bool showPlayOnEmptyFile: true

    signal fileSelected(string newFilePath)
    signal startTestPlay
    signal stopTestPlay

    Label {
        id: toneName
        text: {
            const path = control.filePath
            if (path) {
                if (path.startsWith("file://")) {
                    return path.substring(7)
                }
                return path
            }
            return control.placeholderText
        }
        color: control.filePath ? Theme.primaryTextColor : Theme.secondaryTextColor
        maximumLineCount: 2
        wrapMode: Label.Wrap
        elide: Label.ElideRight
        anchors {
            left: parent.left
            leftMargin: 10
            right: testPlayButton.visible
                   ? testPlayButton.left
                   : (resetToDefaultButton.visible
                      ? resetToDefaultButton.left
                      : pickToneButton.left)
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }

        Accessible.ignored: true
    }

    Button {
        id: testPlayButton
        width: resetToDefaultButton.height
        visible: !!control.filePath || control.showPlayOnEmptyFile
        icon.source: control.isPlaying ? Icons.mediaPlaybackPause : Icons.mediaPlaybackStart
        anchors {
            right: resetToDefaultButton.visible ? resetToDefaultButton.left : pickToneButton.left
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }

        onClicked: () => {
            if (control.isPlaying) {
                control.stopTestPlay()
            } else {
                control.startTestPlay()
            }
        }
    }

    Button {
        id: resetToDefaultButton
        width: resetToDefaultButton.height
        icon.source: Icons.editDelete
        visible: !!control.filePath
        anchors {
            right: pickToneButton.left
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }

        onClicked: () => control.fileSelected("")

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Reset tone")
        Accessible.description: qsTr("Reset the tone to its default option")
        Accessible.focusable: true
        Accessible.onPressAction: () => resetToDefaultButton.click()
    }

    Button {
        id: pickToneButton
        icon.source: Icons.folderOpen
        width: pickToneButton.height
        anchors {
            right: parent.right
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }
        onClicked: () => toneFileDialog.open()

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Pick ring tone")
        Accessible.description: qsTr("Select the ring tone you want to use for incoming calls")
        Accessible.focusable: true
        Accessible.onPressAction: () => pickToneButton.click()
    }

    FileDialog {
        id: toneFileDialog
        onAccepted: control.fileSelected(toneFileDialog.selectedFile)
        nameFilters: FileHelper.mediaFileSelectors(false)
    }
}
