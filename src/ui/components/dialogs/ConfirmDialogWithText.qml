pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

ConfirmDialog {
    id: control

    signal acceptedWithText(string text)

    property string inputLabel
    property alias inputText: inputField.text

    height: control.inputLabel ? 430 : 354
    minimumHeight: control.height
    maximumHeight: control.height

    TextField {
        id: inputField
        visible: !!control.inputLabel
        height: visible ? implicitHeight : 0
        placeholderText: control.inputLabel
        anchors {
            top: control.contentText.top
            topMargin: control.contentText.contentHeight + 20
            left: parent.left
            right: parent.right
            leftMargin: 20
            rightMargin: 20
        }

        Accessible.role: Accessible.EditableText
        Accessible.name: control.inputLabel
        Accessible.focusable: true
        Component.onCompleted: () => { if (inputField.visible) inputField.forceActiveFocus() }
    }

    onAccepted: () => control.acceptedWithText(inputField.text)
}