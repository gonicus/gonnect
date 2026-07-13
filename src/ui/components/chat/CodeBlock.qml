pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    height: codeLabel.implicitHeight + 2 * codeFlick.anchors.margins + codeFlick.bottomMargin

    property alias text: codeLabel.text
    property alias fenceInfo: codeHighlighter.language

    Rectangle {
        id: codeBg
        radius: 8
        anchors.fill: parent
        color: Theme.backgroundSecondaryColor
        border {
            width: 1
            color: Theme.borderColor
        }
    }

    Flickable {
        id: codeFlick
        clip: true
        contentWidth: codeLabel.x + codeLabel.implicitWidth
        contentHeight: codeLabel.implicitHeight
        bottomMargin: codeFlick.contentWidth > codeFlick.width ? 10 : 0
        anchors {
            fill: codeBg
            margins: 10
        }

        ScrollBar.horizontal: ScrollBar {
            id: hScrollbar
            height: 5
            policy: codeFlick.contentWidth > codeFlick.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }

        Column {
            id: lineNumberCol
            width: 30
            height: codeFlick.contentHeight
            anchors {
                left: parent.left
            }

            Repeater {
                model: codeLabel.lineCount
                delegate: Label {
                    id: lineNumDelg
                    text: lineNumDelg.index + 1
                    height: codeLabel.contentHeight / codeLabel.lineCount
                    horizontalAlignment: Label.AlignRight
                    verticalAlignment: Label.AlignVCenter
                    color: Theme.secondaryTextColor
                    font.family: "monospace"
                    anchors {
                        left: parent?.left
                        right: parent?.right
                    }

                    required property int index
                }
            }
        }

        TextEdit {
            id: codeLabel
            readOnly: true
            font.family: "monospace"
            font.pixelSize: Theme.fontPixelSize
            anchors {
                left: lineNumberCol.right
                leftMargin: 12
            }

            CodeHighlighter {
                id: codeHighlighter
                quickDocument: codeLabel.textDocument
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton

            onWheel: (wheel) => {
                         if (wheel.modifiers & Qt.ShiftModifier) {
                             const nextX = codeFlick.contentX - wheel.angleDelta.y
                             const maxX = codeFlick.contentWidth - codeFlick.width
                             codeFlick.contentX = Util.clamp(nextX, 0, maxX)
                             wheel.accepted = true
                         } else {
                             wheel.accepted = false
                         }
                     }
        }
    }

    ClipboardButton {
        id: clipboardButton
        text: control.text
        visible: codeBlockHoverHandler.hovered
        anchors {
            top: codeFlick.top
            right: codeFlick.right
        }
    }

    HoverHandler {
        id: codeBlockHoverHandler
    }
}
