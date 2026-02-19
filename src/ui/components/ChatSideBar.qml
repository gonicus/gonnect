pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property alias chatRoom: chatModel.chatRoom

    readonly property alias messageCount: chatModel.realMessagesCount

    Item {
        id: chatContainer
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: chatInputContainer.top
            bottomMargin: 24
        }

        ListView {
            id: chatListView
            anchors.fill: parent
            clip: true
            bottomMargin: 20
            model: ChatProxyModel {
                ChatModel {
                    id: chatModel
                }
            }

            // Use timer as items needs time for rendering before it can scroll there
            onCountChanged: () => initialScrollTimer.start()
            Component.onCompleted: () => initialScrollTimer.start()

            Timer {
                id: initialScrollTimer
                repeat: false
                running: false
                interval: 200
                onTriggered: () => {
                    chatListView.positionViewAtIndex(chatListView.count - 1, ListView.Center)
                    initialScrollTimer.interval = 5  // Longer interval is only needed on inital load due to longer rendering time
                }
            }

            Accessible.role: Accessible.List
            Accessible.name: qsTr("Chat message list")
            Accessible.description: qsTr("List of all the messages in the current chat")

            delegate: Item {
                id: delg
                height: messageBackground.y + messageBackground.height
                anchors {
                    left: parent?.left
                    right: parent?.right
                    leftMargin: 10
                    rightMargin: 10
                }

                required property int index
                required property string fromId
                required property string nickName
                required property string message
                required property bool isOwnMessage
                required property bool isSystemMessage
                required property date timestamp

                readonly property Item prevDelg: delg.index > 0 ? chatListView.itemAtIndex(delg.index - 1) : null
                readonly property string timeFormatted: Qt.formatTime(delg.timestamp, "hh:mm")

                property color labelColor: Theme.primaryTextColor

                Accessible.role: Accessible.ListItem
                Accessible.name: qsTr("Chat message")
                Accessible.description: qsTr("Selected chat message: ") +
                                        qsTr("Sender: ") + control.nickName +
                                        qsTr("Time: ") + control.timestamp +
                                        qsTr("Message text: ") + control.message
                                        (delg.isSystemMessage ? qsTr("Hint: This is a system message") : "") +
                                        (delg.isOwnMessage ? qsTr("Hint: This is a message you sent") : "")
                Accessible.focusable: true

                states: [
                    State {
                        when: delg.isSystemMessage

                        PropertyChanges {
                            messageBackground.color: "transparent"
                            delg.labelColor: Theme.secondaryTextColor
                            infoLabel.horizontalAlignment: Text.AlignHCenter
                            clipboardButton.visible: false
                        }

                        AnchorChanges {
                            target: infoLabel
                            anchors.left: undefined
                            anchors.horizontalCenter: delg.horizontalCenter
                        }
                        AnchorChanges {
                            target: messageBackground
                            anchors.left: undefined
                            anchors.horizontalCenter: delg.horizontalCenter
                        }
                    },

                    State {
                        when: !delg.isOwnMessage

                        PropertyChanges {
                            messageBackground.color: Theme.accentColor
                            delg.labelColor: Theme.foregroundWhiteColor
                        }

                        AnchorChanges {
                            target: infoLabel
                            anchors.left: undefined
                            anchors.right: delg.right
                        }
                        AnchorChanges {
                            target: messageBackground
                            anchors.left: undefined
                            anchors.right: delg.right
                        }
                        AnchorChanges {
                            target: clipboardButton
                            anchors.right: messageBackground.left
                            anchors.left: undefined
                        }
                    }
                ]

                Label {
                    id: infoLabel
                    y: 20
                    font.pixelSize: 14
                    textFormat: Text.StyledText
                    color: Theme.secondaryTextColor
                    text: delg.nickName ? `${delg.nickName}, ${delgtimeFormatted}` : delg.timeFormatted
                    visible: !delg.prevDelg
                             || delg.prevDelg.fromId !== delg.fromId
                             || delg.prevDelg.timeFormatted !== delg.timeFormatted
                    anchors {
                        left: parent.left
                        leftMargin: 10
                        rightMargin: 10
                    }

                    Accessible.ignored: true
                }

                ClipboardButton {
                    id: clipboardButton
                    visible: chatMessageHoverHandler.hovered
                    text: delg.message.replace(/<.+?>/gm, "")
                    anchors {
                        verticalCenter: messageBackground.verticalCenter
                        left: messageBackground.right
                        rightMargin: 10
                        leftMargin: 10
                    }
                }

                Rectangle {
                    id: messageBackground
                    width: parent.width * 0.8
                    height: msgLabel.implicitHeight + 20
                    color: Theme.borderColor
                    radius: 6
                    anchors {
                        top: infoLabel.visible ? infoLabel.bottom : parent.top
                        left: parent.left
                        topMargin: infoLabel.visible ? 0 : 10
                    }

                    Label {
                        id: msgLabel
                        text: delg.message
                        color: delg.labelColor
                        linkColor: msgLabel.color
                        wrapMode: Label.Wrap
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: 10
                            rightMargin: 10

                            top: parent.top
                            topMargin: 10
                        }

                        onLinkActivated: (link) => {
                            if (!link.startsWith("http")) {
                                Qt.openUrlExternally("https://" + link)
                            } else {
                                Qt.openUrlExternally(link)
                            }
                        }

                        Accessible.ignored: true
                    }
                }

                HoverHandler {
                    id: chatMessageHoverHandler
                }
            }
        }
    }

    Item {
        id: chatInputContainer
        height: chatInputField.y + chatInputField.height + chatInputField.anchors.margins
        anchors {
            left: chatContainer.left
            right: chatContainer.right
            bottom: parent.bottom
        }

        Item {
            id: emojiButton
            height: chatInputField.implicitHeight - 20
            width: emojiButton.height
            anchors {
                verticalCenter: chatInputContainer.verticalCenter
                left: parent.left
                leftMargin: 10
            }

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Select emoji")
            Accessible.focusable: true
            Accessible.onPressAction: () => emojiButton.selectEmoji()

            Label {
                anchors.centerIn: parent
                text: "🙂"
                font.pixelSize: 20

                Accessible.ignored: true
            }

            TapHandler {
                gesturePolicy: TapHandler.WithinBounds
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                exclusiveSignals: TapHandler.SingleTap

                onTapped: () => emojiButton.selectEmoji()
            }

            function selectEmoji() {
                const item = emojiPickerComponent.createObject(emojiButton)
                item.emojiPicked.connect(emoji => chatInputField.insert(chatInputField.cursorPosition, emoji))
                item.open()
            }
        }

        Component {
            id: emojiPickerComponent

            Popup {
                id: emojiPickerPopup
                width: 300
                height: 400
                x: chatInputField.x + chatInputField.width - emojiPickerPopup.width
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
            enabled: !!control.chatRoom
            y: chatInputField.anchors.margins
            placeholderText: qsTr("Enter chat message...")
            anchors {
                left: emojiButton.right
                right: parent.right
                margins: 10
            }

            onAccepted: () => {
                const trimmed = chatInputField.text.trim()
                if (trimmed !== "") {
                    control.chatRoom.sendMessage(trimmed)
                    chatInputField.text = ""
                }
            }

            Accessible.role: Accessible.EditableText
            Accessible.name: chatInputField.placeholderText
            Accessible.focusable: true
        }
    }
}
