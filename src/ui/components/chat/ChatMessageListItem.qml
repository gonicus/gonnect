pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitHeight: messageLabel.visible ? messageLabel.height : messageImage.height

    LoggingCategory {
        id: category
        name: "gonnect.qml.ChatMessageListItem"
        defaultLogLevel: LoggingCategory.Warning
    }

    required property date timestamp
    required property string nickName
    required property string message
    required property string imageUrl

    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("Chat message")
    Accessible.description: qsTr("Selected chat message: ") +
                            qsTr("Sender: ") + control.nickName +
                            qsTr("Time: ") + control.timestamp +
                            qsTr("Message text: ") + control.message
    Accessible.focusable: true

    Component.onCompleted: () => {
        if (!control.message) {
            console.debug(category, "image URL", control.imageUrl)
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

        Accessible.ignored: true
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

        Accessible.ignored: true
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
                switch (messageImage.status) {
                    case Image.Null:
                        console.warn(category, 'image', messageImage.source, 'NULL')
                        break
                    case Image.Ready:
                        console.info(category, 'image', messageImage.source, 'READY')
                        break
                    case Image.Loading:
                        console.info(category, 'image', messageImage.source, 'LOADING')
                        break
                    case Image.Error:
                        console.error(category, 'image', messageImage.source, 'ERROR')
                        break
                }
            }
        }
    }
}
