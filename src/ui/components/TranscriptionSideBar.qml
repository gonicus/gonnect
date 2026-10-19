pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property alias conferenceConnector: chunksModel.conferenceConnector

    ListView {
        id: chunkListView
        anchors.fill: parent
        clip: true
        spacing: Theme.d
        model: TranscriptionChunksModel {
            id: chunksModel
        }

        Accessible.role: Accessible.List
        Accessible.name: qsTr("Transcription")
        Accessible.description: qsTr("Live transcription of the conference")

        onCountChanged: () => Qt.callLater(chunkListView.positionViewAtEnd)

        delegate: Item {
            id: chunkDelg
            height: textColumn.implicitHeight + Theme.d
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property string participantName
            required property string finalText
            required property string stableText
            required property string unstableText
            required property date receivedAt

            readonly property string displayText: chunkDelg.finalText
                                                  ? chunkDelg.finalText
                                                  : (chunkDelg.stableText + chunkDelg.unstableText)

            Accessible.role: Accessible.ListItem
            Accessible.name: qsTr("%1: %2").arg(chunkDelg.participantName).arg(chunkDelg.displayText)

            Column {
                id: textColumn
                spacing: 2
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Row {
                    spacing: Theme.d
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: chunkDelg.participantName
                        font.weight: Font.Medium
                        elide: Label.ElideRight
                    }

                    Label {
                        //: Date format for conference subscription
                        text: Qt.formatDateTime(chunkDelg.receivedAt, qsTr("hh:mm"))
                        color: Theme.secondaryTextColor
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }

                Label {
                    text: chunkDelg.displayText
                    wrapMode: Label.Wrap
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
            }
        }
    }
}
