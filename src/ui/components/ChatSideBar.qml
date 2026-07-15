pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property alias chatRoom: chat.chatRoom
    property alias chatProvider: chat.chatProvider

    function giveFocus() {
        chat.giveFocus()
    }

    Connections {
        target: control.chatProvider
        function onClipboardImageUploaded(imageFilePath, chatRoom) {
            ViewHelper.topDrawer.loader.sourceComponent = imagePreviewComponent

            const item = ViewHelper.topDrawer.loader.item
            item.source = `file://${imageFilePath}`
            item.chatRoom = chatRoom
        }
    }

    Component {
        id: imagePreviewComponent

        ImageSendPreview {}
    }

    Chat {
        id: chat
        showTitleBar: false
        anchors.fill: parent
    }
}
