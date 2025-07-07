pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitHeight: messageLabel.visible ? messageLabel.height : messageImage.height

    required property date dateTime
    required property string senderDisplayName
    required property string message
    required property string imageUrl

    Component.onCompleted: () => {
        if (!control.message) {
            console.error("===>", control.imageUrl)
        }
    }

    Label {
        id: timestampLabel
        color: Theme.secondaryTextColor
        text: `${control.dateTime.toLocaleString(Qt.locale(), "hh:mm:ss dd.MM.yy")} (${control.senderDisplayName})`
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
        anchors {
            top: parent.top
            left: timestampLabel.right
            right: parent.right
            leftMargin: 10
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
}
