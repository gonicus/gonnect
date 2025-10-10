pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitHeight: messageLabel.visible
                    ? (reactionRow.visible
                       ? (reactionRow.y + reactionRow.height + 4)
                       : (messageLabel.y + messageLabel.height))
                    : messageImage.height

    required property string eventId
    required property date timestamp
    required property string nickName
    required property string message
    required property string imageUrl
    required property var reactions

    signal toggleReaction(string emoji)

    Component.onCompleted: () => {
        if (!control.message) {
            console.error("===>", control.imageUrl)
        }
    }

    Label {
        id: timestampLabel
        color: Theme.secondaryTextColor
        text: `${control.timestamp.toLocaleString(Qt.locale(), "hh:mm:ss dd.MM.yy")} (${control.nickName})`
        anchors {
            top: parent.top
            left: parent.left
        }
    }

    Label {
        id: messageLabel
        visible: !!control.message
        text: control.message
        wrapMode: Label.Wrap
        textFormat: Text.MarkdownText
        anchors {
            top: parent.top
            left: timestampLabel.right
            right: parent.right
            leftMargin: 10
        }

        onLinkActivated: link => Qt.openUrlExternally(link)
    }

    Row {
        id: reactionRow
        visible: reactionsRepeater.count > 0
        height: 24
        spacing: 10
        anchors {
            top: messageLabel.bottom
            topMargin: 4
            left: messageLabel.left
        }

        Repeater {
            id: reactionsRepeater
            model: control.reactions
            delegate: ReactionButton {
                id: reactionButton
                emojiText: reactionButton.emoji
                countText: reactionButton.count
                ownSelected: reactionButton.hasOwnReaction
                anchors.verticalCenter: parent?.verticalCenter

                required property string emoji
                required property int count
                required property bool hasOwnReaction

                onClicked: () => control.toggleReaction(reactionButton.emojiText)
            }
        }

        AddReactionButton {
            id: addReactionButton
            anchors.verticalCenter: parent.verticalCenter
            onClicked: () => {
                const item = emojiPickerComponent.createObject(control)
                item.emojiPicked.connect(emoji => {
                                            item.close()
                                            control.toggleReaction(emoji)
                                         })
                item.open()
            }
        }
    }

    Image {
        id: messageImage
        visible: !!control.imageUrl
        source: control.imageUrl
        height: 100
        anchors {
            top: parent.top
            left: timestampLabel.right
            right: parent.right
            leftMargin: 10
        }
        onStatusChanged: () => {
            if (messageImage.visible) {
                console.error('===>', messageImage.source)

                switch (messageImage.status) {
                    case Image.Null:
                        console.error('   ===> NULL')
                        break
                    case Image.Ready:
                        console.error('   ===> READY')
                        break
                    case Image.Loading:
                        console.error('   ===> LOADING')
                        break
                    case Image.Error:
                        console.error('   ===> ERROR')
                        break
                }
            }
        }
    }

    Component {
        id: emojiPickerComponent

        Popup {
            id: emojiPickerPopup
            width: 300
            height: 400
            x: control.mapFromItem(reactionRow, addReactionButton.x, 0).x
            y: control.mapFromItem(reactionRow, 0, addReactionButton.y).y - emojiPickerPopup.height

            signal emojiPicked(string emoji)

            EmojiPicker {
                anchors.fill: parent
                onEmojiPicked: emoji => emojiPickerPopup.emojiPicked(emoji)
            }
        }
    }
}
