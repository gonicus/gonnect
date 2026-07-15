pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import base

ChatMessageAttachmentRectangle {
    id: control
    width: 300
    height: 80

    property ChatMessageContentAudioFile content

    MediaPlayer {
        id: mediaPlayer
        source: control.content?.filePath ?? ""
        audioOutput: AudioOutput {}
    }

    Item {
        id: mainContainer
        anchors {
            left: parent.left
            right: parent.right
            leftMargin: 12
            top: parent.top
            bottom: sliderContainer.top
        }

        RoundButton {
            id: playButton
            flat: true
            icon.source: mediaPlayer.playing ? Icons.mediaPlaybackPause : Icons.mediaPlaybackStart
            width: 40
            height: playButton.width
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
            }

            onClicked: () => {
                if (mediaPlayer.playing) {
                    mediaPlayer.pause()
                } else {
                    mediaPlayer.play()
                }
            }
        }

        Column {
            id: labelCol
            anchors {
                left: playButton.right
                right: downloadButton.left
                rightMargin: 12

                verticalCenter: parent.verticalCenter
                verticalCenterOffset: 2
            }

            Label {
                text: control.content?.fileName ?? ""
                font.pixelSize: 18
                elide: Label.ElideRight
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Label {
                elide: Label.ElideRight
                color: Theme.secondaryTextColor
                font.pixelSize: 12
                text: qsTr("%1:%2 (%3)").arg(Math.floor((mediaPlayer.duration / 1000) / 60))
                                        .arg(Math.floor((mediaPlayer.duration / 1000) % 60).toString().padStart(2, '0'))
                                        .arg(TextFormatHelper.formatFileSize(control.content?.fileSize ?? 0))
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }
        }

        RoundButton {
            id: downloadButton
            flat: true
            icon.source: Icons.documentSave
            width: 40
            height: downloadButton.width
            anchors {
                top: parent.top
                right: parent.right
            }

            onClicked: () => saveFileDialog.open()
        }
    }

    Item {
        id: sliderContainer
        height: 15
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 12
        }

        Slider {
            id: slider
            width: 230
            from: 0
            to: mediaPlayer.duration
            stepSize: 1000
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
            }

            Binding {
                target: slider
                property: "value"
                value: mediaPlayer.position
                when: !slider.pressed  // Prevent updating via binding whilst user is moving handle
            }

            onMoved: () => mediaPlayer.position = slider.value
        }

        Label {
            id: elapsedTimeLabel
            color: Theme.secondaryTextColor
            text: qsTr("%1:%2").arg(Math.floor((mediaPlayer.position / 1000) / 60))
                               .arg(Math.floor((mediaPlayer.position / 1000) % 60).toString().padStart(2, '0'))
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
            }
        }
    }

    FileDialog {
        id: saveFileDialog
        fileMode: FileDialog.SaveFile
        currentFolder: `file://${FileHelper.downloadFolderPath()}`
        selectedFile: `file://${FileHelper.downloadFolderPath()}/${control.content?.fileName ?? ""}`
        onAccepted: () => FileHelper.copyFile(control.content?.filePath ?? "", saveFileDialog.selectedFile)
    }
}
