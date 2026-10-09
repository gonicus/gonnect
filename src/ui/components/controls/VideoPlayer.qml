pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import base

ChatMessageAttachmentRectangle {
    id: control

    implicitWidth: control.cardWidth
    implicitHeight: control.videoHeight + topBar.height + buttonBar.height
    width: control.cardWidth
    height: control.implicitHeight

    property ChatMessageContentVideoFile content
    property alias showFullscreenButton: fullScreenButton.visible
    property real availableHeight: -1
    property real explicitVideoHeight: -1

    readonly property real aspectRatio: (control.sourceSize.width && control.sourceSize.height)
                                        ? (control.sourceSize.width / control.sourceSize.height)
                                        : (1280 / 720)
    readonly property real videoMaxHeight: control.explicitVideoHeight > 0
                                           ? control.explicitVideoHeight
                                           : Math.max(0, ((control.availableHeight > 0
                                                           ? Math.min(220, control.availableHeight)
                                                           : 220)
                                                          - topBar.height - buttonBar.height))
    readonly property real videoHeight: control.videoMaxHeight
    readonly property real videoWidth: control.videoHeight * control.aspectRatio
    readonly property real minButtonBarWidth: buttonBar.leftRowWidth + buttonBar.rightRowWidth + slider.width
    readonly property real cardWidth: Math.max(control.videoWidth, control.minButtonBarWidth)
    readonly property size sourceSize: thumbnail.sourceSize
    readonly property real controlHeight: topBar.height + buttonBar.height

    MediaPlayer {
        id: mediaPlayer
        source: control.content?.filePath ?? ""
        audioOutput: AudioOutput {}
        videoOutput: videoOutput
    }

    Image {
        id: thumbnail
        source: control.content?.thumbnailFilePath ?? ""
        width: control.videoWidth
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
        width: control.cardWidth
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
                leftMargin: Theme.d
                rightMargin: Theme.d
                verticalCenter: parent.verticalCenter
            }
        }
    }

    VideoOutput {
        id: videoOutput
        width: control.videoWidth
        anchors {
            top: topBar.bottom
            bottom: buttonBar.top
            left: parent.left
        }
    }

    BottomButtonBar {
        id: buttonBar
        width: control.cardWidth
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
            anchors.verticalCenter: parent?.verticalCenter

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
