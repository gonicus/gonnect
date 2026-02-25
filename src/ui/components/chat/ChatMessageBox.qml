pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

TextField {
    id: control
    placeholderText: qsTr("Message")

    signal sendMessage

    Keys.onEnterPressed: () => control.sendMessage()
    Keys.onReturnPressed: () => control.sendMessage()

    Accessible.role: Accessible.EditableText
    Accessible.name: qsTr("Type message")
    Accessible.description: qsTr("Enter the chat text message")
    Accessible.focusable: true
}
