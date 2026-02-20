pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

BaseDialog {
    id: control
    title: qsTr("Authentication failed")

    signal passwordAccepted(string password)

    property alias text: contentLabel.text

    Label {
        id: contentLabel
        text: qsTr("Please enter the password:")
        elide: Label.ElideRight
        wrapMode: Label.WordWrap
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 20
        }

        Accessible.role: Accessible.StaticText
        Accessible.name: control.text + contentLabel.text
    }

    TextField {
        id: passwordField
        placeholderText: qsTr("Password")
        echoMode: TextInput.Password
        anchors {
            top: contentLabel.bottom
            topMargin: 20
            left: contentLabel.left
            right: contentLabel.right
        }

        Keys.onEnterPressed: () => okButton.click()
        Keys.onReturnPressed: () => okButton.click()

        Accessible.role: Accessible.EditableText
        Accessible.name: qsTr("Enter the password")
        Accessible.focusable: true
    }

    Button {
        id: okButton
        text: qsTr("Ok")
        highlighted: true
        anchors {
            bottom: parent.bottom
            right: parent.right
            bottomMargin: 5
            rightMargin: 10
        }

        onClicked: () => {
            okButton.enabled = false
            control.passwordAccepted(passwordField.text)
            control.close()
        }

        Accessible.role: Accessible.Button
        Accessible.name: okButton.text
        Accessible.focusable: true
        Accessible.onPressAction: () => okButton.click()
    }
}
