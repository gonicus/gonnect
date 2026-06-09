pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    height: Util.clamp(messageField.contentHeight + messageField.anchors.margins * 2 + buttonBar.height,
                       100,
                       Math.floor(0.8 * parent.height))

    signal sendMessage
    signal sendImage(string filePath)
    signal sendFile(string filePath)
    signal imageFromClipboardReceived
    signal editLastMessage

    property alias chatRoom: userSelectPopup.chatRoom
    property string editMessageId
    property alias text: messageField.text

    readonly property bool hasMessage: !!messageField.text.trim()

    onChatRoomChanged: () => control.clear()

    function clear() {
        messageField.clear()
    }

    // SpellCheckHighlighter {
    //     quickDocument: messageField.textDocument
    // }

    UserSelect {
        id: userSelectPopup

        onAccepted: id => {
            messageField.replaceCurrentWord(id)
            userSelectPopup.close()
        }
    }

    FilteredEmojis {
        id: emojiSelectPopup

        onAccepted: emoji => {
            messageField.replaceCurrentWord(emoji)
            emojiSelectPopup.close()
        }
    }

    Rectangle {
        height: 1
        color: Theme.borderColor
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
    }

    Label {
        text: qsTr("Enter message...")
        color: Theme.secondaryInactiveTextColor
        visible: messageField.text === "" && !messageField.activeFocus
        anchors {
            top: messageField.top
            left: messageField.left
        }
    }

    TextEdit {
        id: messageField
        clip: true
        color: Theme.primaryTextColor
        font.pixelSize: 14
        wrapMode: TextEdit.Wrap
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: buttonBar.top
            margins: 10
        }

        property int lastCursorPosition: 0

        onTextEdited: () => {

            // Find current word at cursor
            const bounds = messageField.currentWordBoundings()
            const currWord = text.substring(bounds[0], bounds[1])

            // Check whether word is a user mention (started or complete)
            if (currWord.startsWith("@")) {
                userSelectPopup.filterText = currWord.substring(1)

                if (userSelectPopup.count) {
                    let topMostItem = control
                    while (topMostItem.parent) {
                        topMostItem = topMostItem.parent
                    }

                    const cursorCoord = messageField.cursorRectangle
                    const globalCoord = topMostItem.mapFromItem(messageField, cursorCoord.x, cursorCoord.y)
                    if (globalCoord.x > topMostItem.width / 2) {
                        userSelectPopup.x = Qt.binding(() => cursorCoord.x - userSelectPopup.width)
                    } else {
                        userSelectPopup.x = cursorCoord.x
                    }

                    userSelectPopup.y = Qt.binding(() => cursorCoord.y - userSelectPopup.height)

                    if (!userSelectPopup.opened) {
                        userSelectPopup.open()
                    }
                } else if (userSelectPopup.opened) {
                    userSelectPopup.close()
                }

            // Check if emoji popup shall be made
            } else if (currWord.startsWith(":") && currWord.length > 2) {
                emojiSelectPopup.filterText = currWord.substring(1)

                if (emojiSelectPopup.count) {
                    let topMostItem = control
                    while (topMostItem.parent) {
                        topMostItem = topMostItem.parent
                    }

                    const cursorCoord = messageField.cursorRectangle
                    const globalCoord = topMostItem.mapFromItem(messageField, cursorCoord.x, cursorCoord.y)
                    if (globalCoord.x > topMostItem.width / 2) {
                        emojiSelectPopup.x = Qt.binding(() => cursorCoord.x - emojiSelectPopup.width)
                    } else {
                        emojiSelectPopup.x = cursorCoord.x
                    }

                    emojiSelectPopup.y = Qt.binding(() => cursorCoord.y - emojiSelectPopup.height)

                    if (!emojiSelectPopup.opened) {
                        emojiSelectPopup.open()
                    }
                } else if (emojiSelectPopup.opened) {
                    emojiSelectPopup.close()
                }

            } else {
                if (userSelectPopup.opened) {
                    userSelectPopup.close()
                }
                if (emojiSelectPopup.opened) {
                    emojiSelectPopup.close()
                }
            }
        }

        Keys.onPressed: (keyEvent) => {
            if (!control.enabled) {
                return
            }

            if (userSelectPopup.opened) {
                // User mentions

                if (keyEvent.key === Qt.Key_Up) {
                    keyEvent.accepted = true
                    userSelectPopup.decrementIndex()
                } else if (keyEvent.key === Qt.Key_Down) {
                    keyEvent.accepted = true
                    userSelectPopup.incrementIndex()
                } else if ([Qt.Key_Enter, Qt.Key_Return].includes(keyEvent.key)) {

                    // Navigate user mention popup
                    if (userSelectPopup.selectedIndex >= 0) {
                        // Insert selected entry
                        keyEvent.accepted = true
                        userSelectPopup.accepted(userSelectPopup.idAt(userSelectPopup.selectedIndex))
                    } else if (userSelectPopup.count === 1) {
                        // Insert sole entry
                        keyEvent.accepted = true
                        userSelectPopup.accepted(userSelectPopup.idAt(0))
                    } else {
                        userSelectPopup.close()
                    }
                }

            } else if (emojiSelectPopup.opened) {
                // Emoji popup

                if (keyEvent.key === Qt.Key_Up) {
                    keyEvent.accepted = true
                    emojiSelectPopup.decrementIndex()
                } else if (keyEvent.key === Qt.Key_Down) {
                    keyEvent.accepted = true
                    emojiSelectPopup.incrementIndex()
                } else if ([Qt.Key_Enter, Qt.Key_Return].includes(keyEvent.key)) {

                    // Navigate emoji popup
                    if (emojiSelectPopup.selectedIndex >= 0) {
                        // Insert selected entry
                        keyEvent.accepted = true
                        emojiSelectPopup.accepted(emojiSelectPopup.emojiAt(emojiSelectPopup.selectedIndex))
                    } else if (emojiSelectPopup.count === 1) {
                        // Insert sole entry
                        keyEvent.accepted = true
                        emojiSelectPopup.accepted(emojiSelectPopup.emojiAt(0))
                    } else {
                        emojiSelectPopup.close()
                    }
                }

            } else if (keyEvent.key === Qt.Key_Up) {

                if (messageField.lastCursorPosition === messageField.cursorPosition) {
                    // Edit last message
                    keyEvent.accepted = true

                    if (!control.editMessageId) {
                        control.editLastMessage()
                    }
                } else {
                    messageField.lastCursorPosition = messageField.cursorPosition
                }

            } else if ([Qt.Key_Down, Qt.Key_Left, Qt.Key_Right].includes(keyEvent.key)) {
                messageField.lastCursorPosition = messageField.cursorPosition

            } else if (keyEvent.key === Qt.Key_Escape && control.editMessageId) {
                control.editMessageId = ""
                messageField.clear()

            } else if (control.hasMessage
                       && [Qt.Key_Enter, Qt.Key_Return].includes(keyEvent.key)
                       && !(keyEvent.modifiers & Qt.ShiftModifier)) {

                // Send message
                control.sendMessage()

            } else if (keyEvent.key === Qt.Key_V && (keyEvent.modifiers & Qt.ControlModifier)) {
                // Clipboard paste

                if (ClipboardHelper.hasImage()) {
                    keyEvent.accepted = true
                    control.imageFromClipboardReceived()
                } else if (ClipboardHelper.hasText()) {
                    keyEvent.accepted = true
                    messageField.paste()
                }
            }
        }

        function insertOrRemove(front : string, back : string) {
            const mf = messageField
            const len = mf.text.length
            const start = mf.selectionStart
            const end = mf.selectionEnd
            const selText = mf.selectedText
            const cursorPos = mf.cursorPosition
            const frontLen = front.length
            const backLen = back.length

            if (start === end) {
                // No selection

                if (cursorPos >= frontLen
                    && len - cursorPos >= backLen
                    && mf.getText(cursorPos - frontLen, cursorPos) === front
                    && mf.getText(cursorPos, cursorPos + backLen) === back) {

                    // Remove
                    mf.remove(cursorPos - frontLen, cursorPos + backLen)
                    mf.cursorPosition = cursorPos - frontLen

                } else {
                    // Insert
                    mf.insert(cursorPos, front + back)
                    mf.cursorPosition = cursorPos + frontLen
                }

            } else {
                // Selection active

                if (start >= frontLen
                    && len - end >= backLen
                    && mf.getText(start - frontLen, start) === front
                    && mf.getText(end, end + backLen) === back) {

                    // Remove
                    mf.remove(end, end + backLen)
                    mf.remove(start - frontLen, start)

                } else {
                    // Insert: surround selection
                    mf.remove(start, end)
                    mf.insert(start, `${front}${selText}${back}`)
                    mf.select(start + frontLen, start + frontLen + selText.length)
                }
            }
        }

        function insertOrReplace(text : string) {
            const mf = messageField
            const start = mf.selectionStart
            const end = mf.selectionEnd

            if (start !== end) {
                mf.remove(start, end)
            }
            mf.insert(start, text)
        }

        function replaceCurrentWord(text : string) {
            const bounds = messageField.currentWordBoundings()
            messageField.remove(bounds[0], bounds[1])
            messageField.insert(bounds[0], text + " ")
        }

        function currentWordBoundings() {
            const cPos = messageField.cursorPosition
            const text = messageField.text
            const lastIndex = text.length - 1
            const isSpace = s => /\s/.test(s)

            let start = cPos
            while (start > 0 && !isSpace(text.charAt(start - 1))) {
                --start
            }

            let end = cPos
            while (end < lastIndex && !isSpace(text.charAt(end))) {
                ++end
            }

            return [start, end]
        }
    }

    BottomButtonBar {
        id: buttonBar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }


        BottomButtonBarButton {
            id: emojiButton
            icon: Icons.smiley
            onClicked: () => {
                const item = ViewHelper.globalEmojiPickerPopup as Popup
                if (item.visible) {
                    item.close()
                } else {
                    item.emojiPicked.connect(emojiButton.onEmojiSelected)
                    item.visibleChanged.connect(emojiButton.onEmojiPopupHide)

                    item.openAt(emojiButton.mapToGlobal(emojiButton.x, emojiButton.y))
                }
            }

            function onEmojiSelected(emoji : string) {
                messageField.insertOrReplace(emoji)
            }

            function onEmojiPopupHide() {
                const item = ViewHelper.globalEmojiPickerPopup as Popup
                if (!item.visible) {
                    item.emojiPicked.disconnect(emojiButton.onEmojiSelected)
                    item.visibleChanged.disconnect(emojiButton.onEmojiPopupHide)
                }
            }
        }
        BottomButtonBarSeparator {}
        BottomButtonBarButton {
            id: boldButton
            icon: Icons.formatTextBold
            onClicked: () => messageField.insertOrRemove("**", "**")
        }
        BottomButtonBarButton {
            id: italicButton
            icon: Icons.formatTextItalic
            onClicked: () => messageField.insertOrRemove("*", "*")
        }
        BottomButtonBarButton {
            id: strikethroughButton
            icon: Icons.formatTextStrikethrough
            onClicked: () => messageField.insertOrRemove("<del>", "</del>")
        }
        BottomButtonBarButton {
            id: inlineCodeButton
            icon: Icons.formatTextCode
            onClicked: () => messageField.insertOrRemove("`", "`")
        }
        BottomButtonBarButton {
            id: codeBlockButton
            icon: Icons.addSubtitle
            onClicked: () => messageField.insertOrRemove("\n> ", "")
        }
        BottomButtonBarSeparator {}
        BottomButtonBarButton {
            id: linkButton
            icon: Icons.link
            onClicked: () => messageField.insertOrRemove("[", "]()")
        }
        BottomButtonBarSeparator {}
        BottomButtonBarButton {
            id: addVideoButton
            icon: Icons.uploadMedia
            onClicked: () => uploadMediaDialog.open()
        }
        BottomButtonBarButton {
            id: addFileButton
            icon: Icons.mailAttachment
            onClicked: () => uploadFileDialog.open()
        }

        FileDialog {
            id: uploadMediaDialog
            nameFilters: FileHelper.mediaFileSelectors(true)
            onAccepted: () => control.sendImage(uploadMediaDialog.selectedFile)
        }

        FileDialog {
            id: uploadFileDialog
            onAccepted: () => control.sendFile(uploadFileDialog.selectedFile)
        }

        rightContent: [
            BottomButtonBarButton {
                id: sendButton
                icon: Icons.documentSend
                enabled: control.hasMessage

                onClicked: () => {
                    if (control.enabled && control.hasMessage) {
                        control.sendMessage()
                    }
                }
            }
        ]
    }

    Accessible.role: Accessible.EditableText
    Accessible.name: qsTr("Type message")
    Accessible.description: qsTr("Enter the chat text message")
    Accessible.focusable: true

}
