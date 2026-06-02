pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control
    implicitWidth: {
        if (control.isStateUpdate) {
            return stateLabel.implicitWidth
        } else if (control.isSimpleText) {
            return messageLabel.implicitWidth
        } else if (control.isImage) {
            return messageImage.sourceSize.width
        } else if (attachmentLoader.item) {
            return attachmentLoader.width
        }
        return 36
    }
    implicitHeight: {
        if (control.isStateUpdate) {
            return stateLabel.implicitHeight
        } else if (control.isSimpleText) {
            return messageLabel.implicitHeight
        } else if (control.isImage) {
            return messageImage.height
        } else if (attachmentLoader.item) {
            return attachmentLoader.height
        }
        return 36
    }

    required property bool isStateUpdate
    required property bool isText
    required property bool isSimpleText
    required property bool isMultiText
    required property bool isImage
    required property bool isFile
    required property bool isAudioFile
    required property bool isVideoFile

    required property int userState
    required property string affectedUserName
    required property string simpleText
    required property list<ChatMessageContentPart> multiText
    required property string imageUrl
    required property string fileUrl
    required property string fileName
    required property int fileSize
    required property string thumbnailFileUrl

    readonly property alias messageLabel: messageLabel

    signal openDirectChatRequested(string userId)

    readonly property bool isShortEmojiOnly: control.isText && ViewHelper.isShortEmojiString(control.message)

    // Text
    TextEdit {
        id: messageLabel
        visible: control.isSimpleText
        text: control.simpleText
        wrapMode: Label.Wrap
        textFormat: Text.MarkdownText
        readOnly: true
        font.pixelSize: control.isShortEmojiOnly ? 48 : 13
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
        source: control.imageUrl
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
        visible: control.isImage
        maskSource: messageImageCornerCropper
        source: messageImage
        anchors.fill: messageImage

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onSingleTapped: () => {
                ViewHelper.showLargeImage(control.imageUrl)
            }
        }
    }

    Loader {
        id: attachmentLoader
        visible: control.isFile || control.isMultiText
        anchors {
            left: parent.left
            top: messageLabel.top
        }
        source: {
            if (control.isAudioFile) {
                return "qrc:/qt/qml/base/ui/components/controls/AudioPlayer.qml"
            } else if (control.isVideoFile) {
                return "qrc:/qt/qml/base/ui/components/controls/VideoPlayer.qml"
            } else if (control.isFile) {
                return "qrc:/qt/qml/base/ui/components/chat/FileAttachment.qml"
            } else if (control.isMultiText) {
                return "qrc:/qt/qml/base/ui/components/chat/MultiText.qml"
            }
            return ""
        }

        Binding {
            target: attachmentLoader.item
            when: !!attachmentLoader.item?.hasOwnProperty("availableWidth")
            property: "availableWidth"
            value: control.width
        }

        Binding {
            target: attachmentLoader.item
            when: !!attachmentLoader.item?.hasOwnProperty("fileUrl")
            property: "fileUrl"
            value: control.fileUrl
        }

        Binding {
            target: attachmentLoader.item
            when: !!attachmentLoader.item?.hasOwnProperty("fileName")
            property: "fileName"
            value: control.fileName
        }

        Binding {
            target: attachmentLoader.item
            when: !!attachmentLoader.item?.hasOwnProperty("fileSize")
            property: "fileSize"
            value: control.fileSize
        }

        Binding {
            target: attachmentLoader.item
            when: !!attachmentLoader.item?.hasOwnProperty("thumbnailFileUrl")
            property: "thumbnailFileUrl"
            value: control.thumbnailFileUrl
        }

        Binding {
            target: attachmentLoader.item
            when: !!attachmentLoader.item?.hasOwnProperty("multiText")
            property: "multiText"
            value: control.multiText
        }
    }
}
