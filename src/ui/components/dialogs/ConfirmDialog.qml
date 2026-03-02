pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

InfoDialog {
    id: control

    signal rejected

    Button {
        id: cancelButton
        text: qsTr("Cancel")
        anchors {
            bottom: parent.bottom
            left: parent.left
            bottomMargin: 5
            leftMargin: 10
        }

        onClicked: () => {
            control.rejected()
            control.close()
        }

        Accessible.role: Accessible.Button
        Accessible.name: cancelButton.text
        Accessible.focusable: true
        Accessible.onPressAction: () => cancelButton.click()
    }
}
