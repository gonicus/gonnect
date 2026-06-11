pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Controls.impl
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control
    implicitHeight: reactionsContainer.visible
                    ? (reactionsContainer.y + reactionsContainer.implicitHeight)
                    : (messageContentItem.y + messageContentItem.height)

    LoggingCategory {
        id: category
        name: "gonnect.qml.ChatMessageListItem"
        defaultLogLevel: LoggingCategory.Warning
    }

    required property string eventId
    required property string roomId
    required property date timestamp
    required property string nickName
    required property string avatarPath
    required property int userState
    required property string affectedUserId
    required property var reactions
    required property QtObject content

    required property bool isOwnMessage
    required property bool isStateUpdate
    required property bool isSameUserAsPrevious
    required property bool isSameMinuteAsPrevious
    required property bool isSameDayAsPrevious

    required property bool hasRelatedMessage
    required property string relatedMessageNickName
    required property QtObject relatedMessageContent
    required property bool relatedMessageIsStateUpdate
    required property int relatedMessageUserState
    required property string relatedMessageAffectedUserId

    property IChatProvider chatProvider

    property string clickedLink

    signal respondTo(string messageId)

    states: [
        State {
            name: "SHOW_DAY_SEPERATOR_AND_NAME"
            extend: "SHOW_NAME"
            when: !control.isSameDayAsPrevious

            PropertyChanges {
                newDaySeparator.visible: true
            }
            AnchorChanges {
                target: avatarImage
                anchors.top: newDaySeparator.bottom
            }
            AnchorChanges {
                target: nameLabel
                anchors.top: newDaySeparator.bottom
            }
            AnchorChanges {
                target: timestampLabel
                anchors.top: newDaySeparator.bottom
            }
        },
        State {
            name: "SHOW_NAME"
            when: !control.isSameUserAsPrevious

            PropertyChanges {
                avatarImage.visible: true
                nameLabel.visible: true
            }
            AnchorChanges {
                target: messageContentItem
                anchors.top: relatedMessageItem.visible ? relatedMessageItem.bottom : nameLabel.bottom
            }
            AnchorChanges {
                target: relatedMessageItem
                anchors.top: nameLabel.bottom
            }
            AnchorChanges {
                target: timestampLabel
                anchors.top: nameLabel.top
            }
        }
    ]

    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("Chat message")
    Accessible.focusable: true
    Accessible.description: qsTr("Selected chat message - from %1, at %2: %3")
                                .arg(control.nickName)
                                .arg(control.timestamp)
                                .arg(control.content instanceof ChatMessageContentText && control.content.isSimpleText
                                     ? control.content.simpleText
                                     : "")

    QtObject {
        id: internal

        function openEmojiPicker(p : point) {
            const item = ViewHelper.globalEmojiPickerPopup as Popup
            if (item.visible) {
                item.close()
            } else {
                item.emojiPicked.connect(internal.onEmojiSelected)
                item.visibleChanged.connect(internal.onEmojiPopupHide)

                item.openAt(p)
            }
        }

        function onEmojiSelected(emoji : string) {
            control.chatProvider.toggleReaction(control.roomId,
                                                control.eventId,
                                                emoji)
            ;(ViewHelper.globalEmojiPickerPopup as Popup).close()
        }

        function onEmojiPopupHide() {
            const item = ViewHelper.globalEmojiPickerPopup as Popup
            if (!item.visible) {
                item.emojiPicked.disconnect(internal.onEmojiSelected)
                item.visibleChanged.disconnect(internal.onEmojiPopupHide)
            }
        }
    }

    Item {
        id: newDaySeparator
        visible: false
        height: 40
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Rectangle {
            id: leftSep
            height: 1
            color: Theme.borderColor
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: 10
                right: dayLabel.left
                rightMargin: 10
            }
        }

        Label {
            id: dayLabel
            text: control.timestamp.toLocaleDateString(Qt.locale(), "dddd, d. MMMM")
            color: Theme.secondaryTextColor
            font {
                weight: Font.DemiBold
                pixelSize: 10
            }
            anchors {
                centerIn: parent
                verticalCenterOffset: -1
            }
        }

        Rectangle {
            id: rightSep
            height: leftSep.height
            color: leftSep.color
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: leftSep.anchors.leftMargin
                left: dayLabel.right
                leftMargin: leftSep.anchors.rightMargin
            }
        }
    }

    AvatarImage {
        id: avatarImage
        visible: false
        size: 36
        source: control.avatarPath
        initials: ViewHelper.initials(control.nickName)
        showPresenceStatus: false
        anchors {
            left: parent.left
            leftMargin: 10
            top: parent.top
            topMargin: 15
        }
    }

    Label {
        id: nameLabel
        visible: false
        text: control.nickName
        elide: Text.ElideRight
        font.weight: Font.Medium
        font.pixelSize: 14
        anchors {
            top: parent.top
            topMargin: 15

            left: avatarImage.right
            right: timestampLabel.left
            leftMargin: 10
            rightMargin: 10
        }
    }

    Label {
        id: timestampLabel
        visible: !control.isSameMinuteAsPrevious || nameLabel.visible
        color: Theme.secondaryTextColor
        text: control.timestamp.toLocaleString(Qt.locale(), "hh:mm")
        font.pixelSize: 12
        anchors {
            top: messageContentItem.top
            right: parent.right
            rightMargin: 10
        }

        Accessible.ignored: true
    }

    ChatMessageListItemRelatedContent {
        id: relatedMessageItem
        visible: control.hasRelatedMessage
        height: 0
        anchors {
            top: parent.top
            topMargin: 10
            left: messageContentItem.left
            right: messageContentItem.right
        }

        onImplicitHeightChanged: () => Qt.callLater(() => relatedMessageItem.height = relatedMessageItem.implicitHeight)

        nickName: control.relatedMessageNickName
        content: control.relatedMessageContent
        isStateUpdate: control.relatedMessageIsStateUpdate
        userState: control.relatedMessageUserState
        affectedUserName: control.chatProvider?.userById(control.relatedMessageAffectedUserId)?.computedName ?? ""
    }

    Rectangle {
        color: Theme.backgroundSecondaryColor
        visible: messageLabelHoverHandler.hovered
        radius: 6
        anchors {
            fill: messageContentItem
            leftMargin: (control.content instanceof ChatMessageContentText) ? -4 : 0
            margins: (control.content instanceof ChatMessageContentImage) ? -4 : 0
        }
    }

    ChatMessageListItemContent {
        id: messageContentItem
        isStateUpdate: control.isStateUpdate
        userState : control.userState
        affectedUserName: control.chatProvider?.userById(control.affectedUserId)?.computedName ?? ""
        content: control.content

        onOpenDirectChatRequested: userId => {
            if (!userId) {
                return
            }

            const roomId = control.chatProvider?.chatRoomIdForUser(userId) ?? ""
            if (roomId) {
                ViewHelper.showChatRoom(control.chatProvider, roomId)
            } else {
                ViewHelper.showCreateRoomDialog(control.chatProvider, [ userId ])
            }
        }

        anchors {
            top: relatedMessageItem.visible ? relatedMessageItem.bottom : parent.top
            left: nameLabel.left
            right: timestampLabel.left
            rightMargin: 10
        }
    }

    HoverHandler {
        id: messageLabelHoverHandler
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => {
            eventPoint.accepted = true
            const p = eventPoint.pressPosition
            const item = control.childAt(p.x, p.y)
            if (item === messageContentItem) {
                const q = messageContentItem.messageLabel.mapFromItem(control, p)
                control.clickedLink = ViewHelper.stripLinkTags(messageContentItem.messageLabel.linkAt(q.x, q.y))
            } else {
                control.clickedLink = ""
            }

            chatRoomMenuComponent.createObject(messageContentItem.messageLabel).popup()
        }
    }

    Component {
        id: chatRoomMenuComponent

        Menu {
            id: chatMessageContextMenu
            onClosed: () => chatMessageContextMenu.destroy()

            Action {
                text: qsTr("Add reaction...")
                icon.source: Icons.smileyAdd
                onTriggered: () => {
                    const menuItem = chatMessageContextMenu.itemAt(0)
                    const coord = menuItem.mapToGlobal(menuItem.width / 2, menuItem.height / 2)
                    internal.openEmojiPicker(coord)
                }
            }

            Action {
                text: qsTr("Copy to clipboard")
                icon.source: Icons.editCopy
                enabled: control.content instanceof ChatMessageContentText || control.content instanceof ChatMessageContentImage
                onTriggered: () => {
                    if (control.content instanceof ChatMessageContentImage) {
                        ClipboardHelper.copyImageToClipboard(control.content?.imagePath)
                    } else if (messageContentItem.messageLabel.selectedText) {
                        messageContentItem.messageLabel.copy()
                    } else {
                        ClipboardHelper.copyToClipboard(control.content?.simpleText ?? "")
                    }
                }
            }

            Action {
                text: qsTr("Copy link to clipboard")
                icon.source: Icons.editCopy
                enabled: !!control.clickedLink
                onTriggered: () => {
                    ClipboardHelper.copyToClipboard(control.clickedLink)
                }
            }

            Action {
                text: qsTr("Remove message...")
                icon.source: Icons.editDelete
                onTriggered: () => {
                    const item = DialogFactory.createConfirmDialog({
                        title: qsTr("Remove message"),
                        text: qsTr("Do you really want to remove this message?")
                    })
                    item.accepted.connect(() => {
                        control.chatProvider.requestRemoveMessage(control.roomId, control.eventId)
                    })
                }
            }

            Action {
                text: qsTr("Edit message...")
                icon.source: Icons.editor
                enabled: control.isOwnMessage
                onTriggered: () => {
                    ViewHelper.showEditMessageDialog(control.chatProvider, control.roomId, control.eventId, control.content?.simpleText ?? "")
                }
            }

            Action {
                text: qsTr("Reply...")
                icon.source: Icons.mailReplyCustom
                onTriggered: () => control.respondTo(control.eventId)
            }
        }
    }

    Flow {
        id: reactionsContainer
        visible: reactionRepeater.count > 0
        spacing: 6
        anchors {
            left: nameLabel.left
            right: timestampLabel.left
            top: messageContentItem.bottom
            topMargin: 12
        }

        Repeater {
            id: reactionRepeater
            model: control.reactions
            delegate: Item {
                id: reactionDelg
                implicitHeight: 24
                implicitWidth: reactionCountLabel.x + reactionCountLabel.implicitWidth + 6

                required property int count
                required property string reaction
                required property bool isOwnReaction

                Rectangle {
                    id: reactionBg
                    radius: 6
                    anchors.fill: parent
                    color: reactionDelg.isOwnReaction
                           ? Theme.backgroundOffsetColor
                           : (reactionDelgHoverHandler.hovered
                              ? Theme.backgroundOffsetHoveredColor
                              : Theme.backgroundSecondaryColor)
                    border {
                        width: 1
                        color: reactionDelg.isOwnReaction
                               ? Theme.highlightColor
                               : (reactionDelgHoverHandler.hovered
                                  ? Theme.borderHeaderIconHovered
                                  : Theme.borderColor)
                    }
                }

                Label {
                    id: reactionLabel
                    text: reactionDelg.reaction
                    font {
                        family: "Noto Color Emoji"
                        pixelSize: 14
                    }
                    anchors {
                        left: parent.left
                        leftMargin: 4
                        verticalCenter: parent.verticalCenter
                        verticalCenterOffset: 1
                    }
                }

                Label {
                    id: reactionCountLabel
                    text: reactionDelg.count
                    anchors {
                        left: reactionLabel.right
                        leftMargin: 4
                        verticalCenter: parent.verticalCenter
                    }
                }

                HoverHandler {
                    id: reactionDelgHoverHandler
                    cursorShape: Qt.PointingHandCursor
                }

                TapHandler {
                    onTapped: () => {
                        if (reactionDelg.isOwnReaction) {
                            control.chatProvider.retractReaction(control.roomId,
                                                                 control.eventId,
                                                                 reactionDelg.reaction)
                        } else {
                            control.chatProvider.addReaction(control.roomId,
                                                             control.eventId,
                                                             reactionDelg.reaction)
                        }
                    }
                }
            }
        }

        AddReactionButton {
            id: addReactionButton

            onClicked: () => {
                internal.openEmojiPicker(reactionsContainer.mapToGlobal(addReactionButton.x, addReactionButton.y))
            }
        }
    }
}
