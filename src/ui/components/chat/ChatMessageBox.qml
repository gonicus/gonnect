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
}
