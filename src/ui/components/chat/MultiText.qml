pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import base

Column {
    id: control
    width: control.availableWidth

    property ChatMessageContentText content
    property int availableWidth

    signal openDirectChatRequested(string userId)

    Repeater {
        id: rep
        model: control.content?.contentParts ?? null
        delegate: Item {
            id: delg
            implicitHeight: delg.isCode
                            ? (codeBlockLoader.y + codeBlockLoader.height)
                            : (textLabel.y + textLabel.height)

            required property int index
            required property ChatMessageContentPart modelData
            readonly property bool isCode: delg.modelData?.isCode ?? false
            readonly property string text: delg.modelData?.text ?? ""
            readonly property string fenceInfo: delg.modelData?.fenceInfo ?? ""

            anchors {
                left: parent?.left
                right: parent?.right
            }

            TextEdit {
                id: textLabel
                visible: !delg.isCode
                y: delg.index > 0 ? 5 : 0
                text: delg.text
                font.pixelSize: Theme.fontPixelSize
                wrapMode: Label.WordWrap
                textFormat: Text.MarkdownText
                readOnly: true
                cursorDelegate: null
                anchors {
                    left: parent.left
                    right: parent.right
                }

                HoverHandler {
                    id: hoverHandler
                    cursorShape: textLabel.hoveredLink !== ""
                                 ? Qt.PointingHandCursor
                                 : Qt.IBeamCursor
                }

                onLinkActivated: link => {
                    if (link.startsWith("chat://")) {
                        control.openDirectChatRequested(link.substring(7))
                    } else {
                        Qt.openUrlExternally(link)
                    }
                }
            }

            Loader {
                id: codeBlockLoader
                active: delg.isCode
                source: "qrc:/qt/qml/base/ui/components/chat/CodeBlock.qml"
                y: 5
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Binding {
                    target: codeBlockLoader.item
                    property: "text"
                    value: delg.text
                }

                Binding {
                    target: codeBlockLoader.item
                    property: "fenceInfo"
                    value: delg.fenceInfo
                }
            }
        }
    }
}
