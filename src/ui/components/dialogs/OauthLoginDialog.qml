pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

BaseDialog {
    id: control
    title: qsTr("Login required")

    signal startOauthLogin()
    signal closeDialog()

    property alias text: contentLabel.text
    property alias statusText: statusLabel.text
    height: 454

    Label {
        id: contentLabel
        text: ""
        elide: Label.ElideRight
        wrapMode: Label.WordWrap
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 20
        }

        Accessible.role: Accessible.StaticText
        Accessible.name: control.title + ", " + contentLabel.text
    }

    Label {
        id: flowLabel
        text: qsTr("To begin the login, press 'Authenticate'. A browser window will open asking you to login to your account and share the required data with GOnnect.")
        elide: Label.ElideRight
        wrapMode: Label.WordWrap
        anchors {
            top: contentLabel.bottom
            left: parent.left
            right: parent.right
            margins: 20
        }

        Accessible.role: Accessible.StaticText
        Accessible.name: control.title + ", " + flowLabel.text
    }

    Button {
        id: startButton
        text: qsTr("Authenticate")
        highlighted: true
        anchors {
            top: flowLabel.bottom
            horizontalCenter: parent.horizontalCenter
            margins: 20
        }

        onClicked: () => {
            startButton.enabled = false
            control.startOauthLogin()
        }

        Accessible.role: Accessible.Button
        Accessible.name: startButton.text
        Accessible.focusable: true
        Accessible.onPressAction: () => startButton.click()
    }

    Label {
        id: statusLabel
        text: ""
        visible: false
        elide: Label.ElideRight
        wrapMode: Label.WordWrap
        anchors {
            top: startButton.bottom
            left: parent.left
            right: parent.right
            margins: 20
        }
    }

    Button {
        id: closeButton
        text: qsTr("Close")
        anchors {
            bottom: parent.bottom
            right: parent.right
            bottomMargin: 5
            rightMargin: 10
        }

        onClicked: () => {
            closeButton.enabled = false
            control.closeDialog()
            control.close()
        }

        Accessible.role: Accessible.Button
        Accessible.name: closeButton.text
        Accessible.focusable: true
        Accessible.onPressAction: () => closeButton.click()
    }

    function setStatus(status : string, canRetry: bool) {
        startButton.enabled = true
        statusLabel.text = status
        statusLabel.visible = true
        startButton.visible = canRetry
        flowLabel.visible = canRetry
    }
}
