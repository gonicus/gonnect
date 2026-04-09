pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    Item {
        id: rttContainer
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: rttInputContainer.top
            bottomMargin: 24
        }

        ListView {
            id: rttListView
            anchors.fill: parent
            clip: true
            bottomMargin: 20
            model: RTTProvider.model // ProxyModel?

            Accessible.role: Accessible.List
            Accessible.name: qsTr("RTT message list")
            Accessible.description: qsTr("List of all the RTT messages of the current call")

            delegate: ItemDelegate {
                width: parent.width
                text: rttListView.model.message

                Accessible.role: Accessible.ListItem
                Accessible.name: qsTr("Chat message")
                Accessible.description: qsTr("Selected chat message from %1 at %2: %3").arg(delg.isSystemMessage ? qsTr("the server") : delg.isOwnMessage ? qsTr("you") : delg.nickName).arg(delg.timestamp).arg(delg.message)
                Accessible.focusable: true

                contentItem: Text {
                    text: model.message
                    color: model.isMe ? "blue" : "gray"
                    font.italic: model.isFinished

                    // TODO: required property ... bindings here
                }
            }
        }
    }

    Item {
        id: rttInputContainer
        height: rttInputField.y + rttInputField.height + rttInputField.anchors.margins
        anchors {
            left: rttContainer.left
            right: rttContainer.right
            bottom: parent.bottom
        }

        TextField {
            id: rttInputField
            placeholderText: "Message..."
            onAccepted: focus = false // needed?

            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                    RTTProvider.call.rttSendLineSeperator();
                    event.accepted = true
                } else if (event.key === Qt.Key_Backspace) {
                    RTTProvider.call.rttSendBackspace()
                } else if (event.key === Qt.Key_G && (event.modifiers & Qt.ControlModifier)) {
                    // Which key should trigger this?
                    RTTProvider.call.rttSendBell()
                    event.accepted = true
                }
            }

            onTextEdited: {
                let lastChar = text.charAt(cursorPosition - 1);
                if (lastChar !== "") {
                    RTTProvider.call.rttSend(lastChar);
                }
            }

            Accessible.role: Accessible.EditableText
            Accessible.name: chatInputField.placeholderText
            Accessible.focusable: true
        }
    }
}
