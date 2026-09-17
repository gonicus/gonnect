pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Item {
    id: control

    required property list<url> fileUrls
    required property IChatRoom chatRoom

    function close() {
        if (control.StackView.view) {
            control.StackView.view.popCurrentItem(StackView.Immediate)
        }
    }

    function accept() {
        if (!control.chatRoom) {
            console.error("chatRoom must be set")
            return
        }

        for (const url of control.fileUrls) {
            control.chatRoom.sendFile(url.toString())
        }
    }

    Label {
        id: topLabel
        text: qsTr("Shall the following file(s) be sent? (%1)", "", control.fileUrls.length)
              .arg(TextFormatHelper.formatFileSize(FileHelper.fileSizesFromPaths(control.fileUrls)))
        font.pixelSize: 16
        wrapMode: Label.Wrap
        horizontalAlignment: Label.AlignHCenter
        anchors {
            top: parent.top
            topMargin: 20
            left: parent.left
            right: parent.right
        }
    }

    ListView {
        id: fileUrlListView
        clip: true
        anchors {
            left: parent.left
            right: parent.right
            top: topLabel.bottom
            topMargin: 20
            bottom: buttonRow.top
            bottomMargin: 20
        }
        model: control.fileUrls

        ScrollBar.vertical: ScrollBar { width: 5 }

        delegate: Item {
            id: delg

            required property url modelData

            height: 50
            anchors {
                left: parent?.left
                right: parent?.right
                leftMargin: 20
                rightMargin: 20
            }

            Label {
                id: fileNameLabel
                text: FileHelper.fileNameFromPath(delg.modelData)
                font {
                    weight: Font.DemiBold
                    pixelSize: 16
                }
                anchors {
                    left: parent.left
                    bottom: delg.verticalCenter
                }
            }

            Label {
                id: fileSizeLabel
                text: `(${TextFormatHelper.formatFileSize(FileHelper.fileSizeFromPath(delg.modelData))})`
                anchors {
                    left: fileNameLabel.right
                    leftMargin: 5
                    bottom: delg.verticalCenter
                }
            }

            Label {
                id: filePathLabel
                text: delg.modelData.toString()
                elide: Label.ElideRight
                font.italic: true
                anchors {
                    top: delg.verticalCenter
                    left: parent.left
                    right: parent.right
                }
            }
        }
    }

    Row {
        id: buttonRow
        height: okButton.implicitHeight
        spacing: 20
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }

        Button {
            text: qsTr("Cancel")
            onClicked: () => control.close()
        }

        Button {
            id: okButton
            highlighted: true
            text: qsTr("Ok")
            onClicked: () => {
                           control.accept()
                           control.close()
                       }
        }
    }
}
