pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

BaseDialog {
    id: control
    height: 200
    title: qsTr("Upgrade to Jisti Meet")

    signal accepted(bool continuePhoneCall)
    signal rejected

    Label {
        id: contentLabel
        elide: Label.ElideRight
        wrapMode: Label.WordWrap
        text: qsTr("Your call partner would like to continue this call in Jitsi. Do you want to open Jitsi in your standard browser now?")
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 20
        }
    }

    CheckBox {
        id: continueCallCheckBox
        checked: false
        text: qsTr("Keep the current phone call active")
        anchors {
            top: contentLabel.bottom
            margins: 20
            horizontalCenter: parent.horizontalCenter
        }
    }

    Button {
        id: noButton
        text: qsTr("No")
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
    }

    Button {
        id: yesButton
        text: qsTr("Yes")
        highlighted: true
        anchors {
            bottom: parent.bottom
            right: parent.right
            bottomMargin: 5
            rightMargin: 10
        }

        onClicked: () => {
            control.accepted(continueCallCheckBox.checked)
            control.close()
        }
    }
}
