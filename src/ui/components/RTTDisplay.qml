pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    Item {
        id: rttChatContainer
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
            model: RTTProvider.model
            spacing: 12

            Accessible.role: Accessible.List
            Accessible.name: qsTr("RTT message list")
            Accessible.description: qsTr("List of all the RTT messages of the current call")

            onCountChanged: () => Qt.callLater(rttListView.positionViewAtEnd)

            delegate: Item {
                id: rttDelg
                width: rttListView.width
                height: rttBubble.height

                property int maxBubbleSize: rttDelg.width * 0.8

                required property int timestamp
                required property string message
                required property bool isMe
                required property bool isFinished

                Accessible.role: Accessible.ListItem
                Accessible.name: qsTr("RTT message")
                Accessible.description: qsTr("Selected RTT message from %1: %2").arg(rttDelg.isMe ? qsTr("you") : qsTr("call participant")).arg(rttDelg.message)
                Accessible.focusable: true

                Rectangle {
                    id: rttBubble
                    width: Math.min(rttMessage.implicitWidth, rttDelg.maxBubbleSize)
                    height: rttMessage.height
                    radius: 8

                    onHeightChanged: () => Qt.callLater(rttListView.positionViewAtEnd)

                    anchors.right: rttDelg.isMe ? parent.right : undefined
                    anchors.left: !rttDelg.isMe ? parent.left : undefined

                    color: rttDelg.isMe ? Theme.rttBubbleSelf : Theme.rttBubbleOther

                    Label {
                        id: rttMessage
                        text: rttDelg.message
                        color: rttDelg.isMe ? Theme.rttTextSelf : Theme.rttTextOther
                        width: parent.width
                        wrapMode: Label.Wrap
                        padding: 10
                        anchors {
                            centerIn: parent
                        }
                    }

                    Accessible.ignored: true
                }
            }
        }
    }

    Item {
        id: rttInputContainer
        height: rttInputField.y + rttInputField.height + rttInputField.anchors.margins
        anchors {
            left: rttChatContainer.left
            right: rttChatContainer.right
            bottom: parent.bottom
        }

        property bool newMessage: true

        Timer {
            id: rttTimeoutTimer
            interval: 6000
            repeat: true

            onTriggered: () => {
                rttListView.model.updateMessage(rttInputField.text, true, true)
                rttInputContainer.newMessage = true
                rttInputField.text = ""
            }
        }

        TextField {
            id: rttInputField
            placeholderText: "Message..."
            anchors {
                left: parent.left
                right: parent.right
            }

            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                    event.accepted = true

                    rttListView.model.updateMessage(rttInputField.text, true, true)
                    rttInputContainer.newMessage = true
                    rttInputField.text = ""
                    rttTimeoutTimer.restart()

                    RTTProvider.rttSendLineSeperator()
                } else if (event.key === Qt.Key_Backspace) {
                    event.accepted = true

                    rttInputField.text = rttInputField.text.slice(0, -1)
                    rttListView.model.updateMessage(rttInputField.text, true, false)

                    RTTProvider.rttSendBackspace()
                } else if (event.key === Qt.Key_G && (event.modifiers & Qt.ControlModifier)) {
                    // TODO: Which key should trigger this?
                    event.accepted = true

                    RTTProvider.rttSendBell()
                }
            }

            onTextEdited: {
                let lastChar = rttInputField.text.charAt(rttInputField.length - 1);
                if (lastChar !== "") {
                    if (rttInputContainer.newMessage) {
                        rttListView.model.addMessage(Date.now(), rttInputField.text, true)
                        rttInputContainer.newMessage = false
                    } else {
                        rttListView.model.updateMessage(rttInputField.text, true, false)
                    }

                    rttTimeoutTimer.restart()

                    RTTProvider.rttSend(lastChar)
                }
            }

            Accessible.role: Accessible.EditableText
            Accessible.name: rttInputField.placeholderText
            Accessible.focusable: true
        }
    }
}
