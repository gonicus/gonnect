pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Effects
import base

Item {
    id: control
    implicitHeight: {
        if (!control.content) {
            return 0
        } else if (control.isStateUpdate) {
            return stateLabel.implicitHeight
        } else if (control.isRemoved || (control.isText && control.content.isSimpleText)) {
            return messageLabel.implicitHeight
        } else if (control.content instanceof ChatMessageContentImage) {
            return messageLoader.height
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
    property real maxContentHeight: -1
    property bool isPending: false

    readonly property alias messageLabel: messageLabel
    readonly property bool isText: (control.content instanceof ChatMessageContentText)
    readonly property bool isRemoved: (control.content instanceof ChatMessageContentRemoved)
    readonly property bool isShortEmojiOnly: control.isText && ViewHelper.isShortEmojiString(control.content.simpleText)

    signal openDirectChatRequested(string userId)

    // Text
    TextEdit {
        id: messageLabel
        visible: control.isRemoved || (control.isText && control.content.isSimpleText)
        text: {
            if (control.isRemoved) {
                const reason = control.content.reason
                if (reason !== "") {
                    return qsTr("Message has been removed. Reason: %1").arg(reason)
                } else {
                    return qsTr("Message has been removed.")
                }
            } else if (control.isText) {
                return control.content.htmlText
            }
            return ""
        }
        color: control.textColor
        wrapMode: Label.Wrap
        textFormat: Text.RichText
        readOnly: true
        font {
            pixelSize: control.isShortEmojiOnly ? 48 : Theme.fontSizeNormal
            italic: control.isRemoved
        }
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
            pixelSize: Theme.fontSizeSmall
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
    // AnimatedImage cannot do autoTransform and is slower - so only use if really needed for animated image
    Loader {
        id: messageLoader
        visible: false
        active: control.content instanceof ChatMessageContentImage
        sourceComponent: FileHelper.isAnimatedImage(messageLoader.imagePath) ? animatedImageComponent : imageComponent

        readonly property url imagePath: control.content?.imagePath ?? ""
        readonly property int maxWidth: parent.width
        readonly property int maxHeight: control.maxContentHeight > 0 ? Math.min(200, control.maxContentHeight) : 200

        readonly property int paintedWidth: messageLoader.item?.paintedWidth ?? 0
        readonly property int paintedHeight: messageLoader.item?.paintedHeight ?? 0

        anchors {
            top: messageLabel.top
            left: messageLabel.left
        }

        Component {
            id: imageComponent

            Image {
                id: messageImage
                autoTransform: true
                source: messageLoader.imagePath
                width: Math.min(messageImage.sourceSize.width, messageLoader.maxWidth)
                height: Math.min(messageImage.sourceSize.height, messageLoader.maxHeight)
                fillMode: Image.PreserveAspectFit
                verticalAlignment: Image.AlignTop
                horizontalAlignment: Image.AlignLeft
            }
        }

        Component {
            id: animatedImageComponent

            AnimatedImage {
                id: animatedMessageImage
                autoTransform: true
                source: messageLoader.imagePath
                width: Math.min(animatedMessageImage.sourceSize.width, messageLoader.maxWidth)
                height: Math.min(animatedMessageImage.sourceSize.height, messageLoader.maxHeight)
                fillMode: Image.PreserveAspectFit
                verticalAlignment: Image.AlignTop
                horizontalAlignment: Image.AlignLeft
            }
        }
    }


    // Load on demand
    Loader {
        id: messageImageMask
        active: control.content instanceof ChatMessageContentImage
        anchors.fill: messageLoader

        sourceComponent: Item {
            Rectangle {
                id: messageImageCornerCropper
                visible: false
                anchors.fill: parent
                radius: 8
                antialiasing: true
                layer {
                    enabled: true
                    smooth: true
                }
            }

            MultiEffect {
                id: messageImageOpacityMask
                anchors.fill: parent
                source: messageLoader
                maskSource: messageImageCornerCropper
                maskEnabled: true
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                TapHandler {
                    onSingleTapped: () => {
                        ViewHelper.showLargeImage(control.content?.imagePath ?? "")
                    }
                }
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

        Binding {
            target: attachmentLoader.item
            when: !!attachmentLoader.item?.hasOwnProperty("availableHeight")
            property: "availableHeight"
            value: control.maxContentHeight
        }

        Connections {
            target: attachmentLoader.item
            ignoreUnknownSignals: true
            function openDirectChatRequested(userId : string) {
                control.openDirectChatRequested(userId)
            }
        }
    }

    // Load on demand
    Loader {
        id: uploadingOverlay
        active: control.isPending && (control.content instanceof ChatMessageContentImage
                                      || control.content instanceof ChatMessageContentFile
                                      || control.content instanceof ChatMessageContentAudioFile
                                      || control.content instanceof ChatMessageContentVideoFile)
        x: uploadingOverlay.hasAttachment ? attachmentLoader.x : messageLoader.x
        y: uploadingOverlay.hasAttachment ? attachmentLoader.y : messageLoader.y
        width: uploadingOverlay.hasAttachment ? attachmentLoader.item.width : messageLoader.paintedWidth
        height: uploadingOverlay.hasAttachment ? attachmentLoader.item.height : messageLoader.paintedHeight
        z: 100

        readonly property bool hasAttachment: attachmentLoader.visible && !!attachmentLoader.item

        sourceComponent: Item {
            Rectangle {
                id: uploadingBgRect
                anchors.fill: parent
                radius: 8
                color: Theme.backgroundColor
                opacity: 0.9
            }

            Row {
                anchors.centerIn: parent
                spacing: Theme.d / 2

                BusyIndicator {
                    running: true
                    width: 3 * Theme.d
                    height: 3 * Theme.d
                    circleColor: Theme.pickForegroundColor(uploadingBgRect.color)
                    anchors.verticalCenter: parent.verticalCenter
                }

                Label {
                    text: qsTr("Uploading...")
                    color: Theme.pickForegroundColor(uploadingBgRect.color)
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
