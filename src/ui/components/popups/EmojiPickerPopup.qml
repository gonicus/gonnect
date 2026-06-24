pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Controls.impl
import QtQuick.Window
import base

Popup {
    id: control
    width: 300
    contentHeight: quickEmojiRow.height + (internal.isExpanded
                                           ? (separator.height + emojiPickerLoader.height)
                                           : 0)

    signal emojiPicked(string emoji)

    /// Open this popup at the given coordinates. It will position itself automatically.
    /// Must be global coordinates inside GonnectWindow.
    function openAt(p : point) {
        const overlay = control.Overlay.overlay

        if (!overlay) {
            console.error(category, "Cannot show EmojiPickerPopup without attached overlay")
            return
        }

        internal.lastPos = p

        const w = overlay.width / 2
        const h = overlay.height / 2

        if (p.x < w && p.y < h) {
            internal.corner = Qt.TopLeftCorner
        } else if (p.x > w && p.y < h) {
            internal.corner = Qt.TopRightCorner
        } else if (p.x < w && p.y > h) {
            internal.corner = Qt.BottomLeftCorner
        } else {
            internal.corner = Qt.BottomRightCorner
        }

        internal.adjustPosition()

        if (!control.opened) {
            control.open()
        }
    }

    QtObject {
        id: internal

        property bool isExpanded: false
        property int corner: Qt.TopLeftCorner
        property point lastPos: Qt.point(0, 0)

        onIsExpandedChanged: () => Qt.callLater(internal.adjustPosition)

        function adjustPosition() {
            const w = control.width
            const h = control.contentHeight

            switch (internal.corner) {

            case Qt.TopRightCorner:
                control.x = internal.lastPos.x - w
                control.y = internal.lastPos.y
                break

            case Qt.BottomLeftCorner:
                control.x = internal.lastPos.x
                control.y = internal.lastPos.y - h
                break

            case Qt.BottomRightCorner:
                control.x = internal.lastPos.x - w
                control.y = internal.lastPos.y - h
                break

            case Qt.TopLeftCorner:
            default:
                control.x = internal.lastPos.x
                control.y = internal.lastPos.y
                break
            }
        }
    }

    LoggingCategory {
        id: category
        name: "gonnect.qml.EmojiPickerPopup"
        defaultLogLevel: LoggingCategory.Warning
    }

    Row {
        id: quickEmojiRow
        height: 30

        Repeater {
            model: [ "👍", "👎", "✅", "🙂", "🤣", "😢", "❤️", "👏" ]
            delegate: EmojiButton {
                id: quickDelg
                emojiChar: quickDelg.modelData

                required property string modelData

                onClicked: () => control.emojiPicked(quickDelg.modelData)
            }
        }
    }

    Item {
        id: expandButton
        width: 30
        height: 30
        anchors {
            right: parent.right
            verticalCenter: quickEmojiRow.verticalCenter
        }

        Rectangle {
            anchors.fill: parent
            color: Theme.backgroundOffsetHoveredColor
            visible: expandButtonHoverHandler.hovered
        }

        IconLabel {
            id: expandButtonLabel
            anchors.fill: parent
            icon.source: internal.isExpanded ? Icons.arrowUp : Icons.arrowDown
        }

        HoverHandler {
            id: expandButtonHoverHandler
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onSingleTapped: () => {
                internal.isExpanded = !internal.isExpanded
                emojiPickerLoader.active = true
            }
        }
    }

    Item {
        id: separator
        visible: internal.isExpanded
        height: 10
        anchors {
            top: quickEmojiRow.bottom
            left: parent.left
            right: parent.right
        }

        Rectangle {
            color: Theme.borderColor
            height: 1
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: parent.right
            }
        }
    }

    Loader {
        id: emojiPickerLoader
        height: 400
        active: false
        visible: internal.isExpanded
        anchors {
            left: parent.left
            right: parent.right
            top: separator.bottom
        }

        sourceComponent: EmojiPicker {
            onEmojiPicked: emoji => control.emojiPicked(emoji)
        }
    }

    Item {
        anchors.fill: emojiPickerLoader
        visible: internal.isExpanded && emojiPickerLoader.status !== Loader.Ready

        IconLabel {
            anchors.centerIn: parent
            icon {
                source: Icons.viewMoreHorizontalSymbolic
                width: 75
                height: 75
            }
        }
    }
}
