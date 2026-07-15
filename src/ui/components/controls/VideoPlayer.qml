pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import base

ChatMessageAttachmentRectangle {
    id: control
    implicitWidth: control.sourceSize.width
    implicitHeight: topBar.height + control.sourceSize.height + buttonBar.height
    height: 220

    property ChatMessageContentVideoFile content
    property alias showFullscreenButton: fullScreenButton.visible

    readonly property size sourceSize: thumbnail.sourceSize

    MediaPlayer {
        id: mediaPlayer
        source: control.content?.filePath ?? ""
        audioOutput: AudioOutput {}
        videoOutput: videoOutput
    }

    Image {
        id: thumbnail
        source: control.content?.thumbnailFilePath ?? ""
        width: (thumbnail.sourceSize.width && thumbnail.sourceSize.height)
               ? (thumbnail.sourceSize.width / thumbnail.sourceSize.height * thumbnail.height)
               : (1280 / 720)
        visible: !mediaPlayer.playing
        anchors {
            top: topBar.bottom
            bottom: buttonBar.top
            left: parent.left
        }
    }

    Rectangle {
        id: topBar
        height: buttonBar.height
        width: thumbnail.width
        color: buttonBar.color
        topLeftRadius: buttonBar.bottomLeftRadius
        topRightRadius: buttonBar.bottomRightRadius
        anchors {
            top: parent.top
            left: parent.left
        }

        Label {
            text: qsTr("%1 (%2)")
                      .arg((control.content?.fileName ?? "") || (control.content?.filePath ?? ""))
                      .arg(TextFormatHelper.formatFileSize(control.content?.fileSize ?? 0))
            elide: Label.ElideRight
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 12
                rightMargin: 12
                verticalCenter: parent.verticalCenter
            }
        }
    }

    VideoOutput {
        id: videoOutput
        width: (videoOutput.sourceRect.width && videoOutput.sourceRect.height)
               ? (videoOutput.sourceRect.width / videoOutput.sourceRect.height * videoOutput.height)
               : (1280 / 720)
        anchors {
            top: topBar.bottom
            bottom: buttonBar.top
            left: parent.left
        }
    }

    BottomButtonBar {
        id: buttonBar
        width: thumbnail.width
        anchors {
            left: parent.left
            bottom: parent.bottom
        }

        BottomButtonBarButton {
            id: playPauseButton
            icon: mediaPlayer.playing ? Icons.mediaPlaybackPause : Icons.mediaPlaybackStart
            onClicked: () => {
                if (mediaPlayer.playing) {
                    mediaPlayer.pause()
                } else {
                    mediaPlayer.play()
                }
            }
        }

        Label {
            anchors.verticalCenter: parent?.verticalCenter
            text: qsTr("%1:%2 / %3:%4").arg(Math.floor((mediaPlayer.position / 1000) / 60))
                                       .arg(Math.floor((mediaPlayer.position / 1000) % 60).toString().padStart(2, '0'))
                                       .arg(Math.floor((mediaPlayer.duration / 1000) / 60))
                                       .arg(Math.floor((mediaPlayer.duration / 1000) % 60).toString().padStart(2, '0'))
        }

        centerContent: Slider {
            id: slider
            width: 230
            from: 0
            to: mediaPlayer.duration
            stepSize: 1000
            anchors {
                left: parent?.left
                right: parent?.right
                verticalCenter: parent?.verticalCenter
            }

            Binding {
                target: slider
                property: "value"
                value: mediaPlayer.position
                when: !slider.pressed  // Prevent updating via binding whilst user is moving handle
            }

            onMoved: () => mediaPlayer.position = slider.value
        }

        rightContent: [
            BottomButtonBarButton {
                id: saveButton
                icon: Icons.documentSave
                onClicked: () => saveFileDialog.open()
            },

            BottomButtonBarButton {
                id: fullScreenButton
                icon: Icons.viewFullscreen
                onClicked: () => ViewHelper.showLargeVideo(control.content)
            }
        ]
    }

    FileDialog {
        id: saveFileDialog
        fileMode: FileDialog.SaveFile
        currentFolder: `file://${FileHelper.downloadFolderPath()}`
        selectedFile: `file://${FileHelper.downloadFolderPath()}/${control.content?.fileName ?? ""}`
        onAccepted: () => FileHelper.copyFile(control.content?.filePath ?? "", saveFileDialog.selectedFile)
    }
}
