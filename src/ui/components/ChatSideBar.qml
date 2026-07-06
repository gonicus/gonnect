pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    property alias chatRoom: chat.chatRoom
    property alias chatProvider: chat.chatProvider

    Chat {
        id: chat
        showTitleBar: false
        anchors.fill: parent
    }
}
