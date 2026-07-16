pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control
    implicitWidth: {
        if (!control.content) {
            return 0
        } else if (control.isStateUpdate) {
            return stateLabel.implicitWidth
        } else if (control.content instanceof ChatMessageContentText && control.content.isSimpleText) {
            return messageLabel.implicitWidth
        } else if (control.content instanceof ChatMessageContentImage) {
            return messageImage.sourceSize.width
        } else if (attachmentLoader.item) {
            return attachmentLoader.width
        }
        return 36
    }
    implicitHeight: {
        if (!control.content) {
            return 0
        } else if (control.isStateUpdate) {
            return stateLabel.implicitHeight
        } else if (control.content instanceof ChatMessageContentText && control.content.isSimpleText) {
            return messageLabel.implicitHeight
        } else if (control.content instanceof ChatMessageContentImage) {
            return messageImage.height
        } else if (attachmentLoader.item) {
            return attachmentLoader.height
        }
        return 36
    }

    required property QtObject content
    required property bool isStateUpdate
    required property int userState
    required property string affectedUserName

    property color textColor: Theme.primaryTextColor

    readonly property alias messageLabel: messageLabel

    signal openDirectChatRequested(string userId)

    readonly property bool isShortEmojiOnly: control.content instanceof ChatMessageContentText
                                             && ViewHelper.isShortEmojiString(control.content.simpleText)

    // Text
    TextEdit {
        id: messageLabel
        visible: control.content instanceof ChatMessageContentText && control.content.isSimpleText
        text: control.content instanceof ChatMessageContentText ? control.content.simpleText : ""
        color: control.textColor
        wrapMode: Label.Wrap
        textFormat: Text.MarkdownText
        readOnly: true
        font.pixelSize: control.isShortEmojiOnly ? 48 : Theme.fontPixelSize
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            rightMargin: 10
        }

        cursorDelegate: null
    
        HoverHandler {
            id: hoverHandler
            cursorShape: messageLabel.hoveredLink !== "" 
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

    // State
    Label {
        id: stateLabel
        visible: control.isStateUpdate
        color: Theme.secondaryTextColor
        text: EnumTranslation.userStateChange(control.userState, control.affectedUserName)
        font {
            pixelSize: 12
            italic: true
        }
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            rightMargin: 10
        }
    }

    // Image
    AnimatedImage {
        id: messageImage
        visible: false
        source: control.content?.imagePath ?? ""
        height: Math.min(messageImage.sourceSize.height, 200)
        width: Math.min(messageImage.sourceSize.width, parent.width)
        fillMode: Image.PreserveAspectFit
        verticalAlignment: Image.AlignTop
        horizontalAlignment: Image.AlignLeft
        anchors {
            top: messageLabel.top
            left: messageLabel.left
        }
    }

    Rectangle {
        id: messageImageCornerCropper
        visible: false
        anchors.fill: messageImage
        radius: 8
    }

    OpacityMask {
        id: messageImageOpacityMask
        visible: control.content instanceof ChatMessageContentImage
        maskSource: messageImageCornerCropper
        source: messageImage
        anchors.fill: messageImage

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onSingleTapped: () => {
                ViewHelper.showLargeImage(control.content?.imagePath ?? "")
            }
        }
    }

    Loader {
        id: attachmentLoader
        visible: control.content instanceof ChatMessageContentFile
                 || (control.content instanceof ChatMessageContentText && !control.content.isSimpleText)
        anchors {
            left: parent.left
            top: messageLabel.top
        }
        source: {
            if (control.content instanceof ChatMessageContentAudioFile) {
                return "qrc:/qt/qml/base/ui/components/controls/AudioPlayer.qml"
            } else if (control.content instanceof ChatMessageContentVideoFile) {
                return "qrc:/qt/qml/base/ui/components/controls/VideoPlayer.qml"
            } else if (control.content instanceof ChatMessageContentFile) {
                return "qrc:/qt/qml/base/ui/components/chat/FileAttachment.qml"
            } else if (control.content instanceof ChatMessageContentText) {
                return "qrc:/qt/qml/base/ui/components/chat/MultiText.qml"
            }
            return ""
        }

        Binding {
            target: attachmentLoader.item
            property: "content"
            value: control.content
        }

        Binding {
            target: attachmentLoader.item
            when: !!attachmentLoader.item?.hasOwnProperty("availableWidth")
            property: "availableWidth"
            value: control.width
        }

        Connections {
            target: attachmentLoader.item
            ignoreUnknownSignals: true
            function openDirectChatRequested(userId : string) {
                control.openDirectChatRequested(userId)
            }
        }
    }
}
