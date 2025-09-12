pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    height: chatInputField.y + chatInputField.height + chatInputField.anchors.margins

    signal sendMessage(string message)

    function clear() {
        chatInputField.clear()
    }

    Item {
        id: emojiButton
        height: chatInputField.implicitHeight - 20
        width: emojiButton.height
        anchors {
            verticalCenter: control.verticalCenter
            left: parent.left
            leftMargin: 10
        }

        Label {
            anchors.centerIn: parent
            text: "🙂"
            font.pixelSize: 20
        }

        TapHandler {
            gesturePolicy: TapHandler.WithinBounds
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            exclusiveSignals: TapHandler.SingleTap

            onTapped: () => {
                const item = emojiPickerComponent.createObject(emojiButton)
                item.emojiPicked.connect(emoji => chatInputField.insert(chatInputField.cursorPosition, emoji))
                item.open()
            }
        }
    }

    Component {
        id: emojiPickerComponent

        Popup {
            id: emojiPickerPopup
            width: 300
            height: 400
            x: emojiButton.x
            y: chatInputField.y - emojiPickerPopup.height

            signal emojiPicked(string emoji)

            EmojiPicker {
                anchors.fill: parent
                onEmojiPicked: emoji => emojiPickerPopup.emojiPicked(emoji)
            }
        }
    }

    TextField {
        id: chatInputField
        y: chatInputField.anchors.margins
        placeholderText: qsTr("Message")
        anchors {
            left: emojiButton.right
            right: parent.right
            margins: 10
        }

        onAccepted: () => {
            const trimmed = chatInputField.text.trim()
            if (trimmed !== "") {
                control.sendMessage(trimmed)
                chatInputField.text = ""
            }
        }
    }
}
