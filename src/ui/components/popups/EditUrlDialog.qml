pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 500
    implicitHeight: 320

    property IChatRoom chatRoom

    Keys.onReturnPressed: () => internal.commitChanges()
    Keys.onEnterPressed:  () => internal.commitChanges()

    onChatRoomChanged: () => {
                           if (control.chatRoom) {
                               urlField.text = control.chatRoom.conferenceUrl
                           }
                       }

    QtObject {
        id: internal

        readonly property string trimmedUrl: urlField.text.trim()
        readonly property bool isModified: internal.trimmedUrl !== control.url

        function commitChanges() {
            if (saveButton.enabled && internal.isModified) {
                control.chatRoom.requestSetConferenceUrl(internal.trimmedUrl)
                internal.close()
            }
        }

        function close() {
            if (control.StackView.view) {
                control.StackView.view.popCurrentItem(StackView.Immediate)
            }
        }
    }

    HeaderIconButton {
        id: closeButton
        iconSource: Icons.mobileCloseApp
        anchors {
            top: parent.top
            right: parent.right
        }

        onClicked: () => internal.close()
    }

    Label {
        id: descriptionLabel
        wrapMode: Label.Wrap
        text: qsTr("URL of the conference that is permanently associated to this room.")
        anchors {
            top: parent.top
            topMargin: 20
            left: parent.left
            right: closeButton.left
            margins: 20
        }
    }

    TextField {
        id: urlField
        anchors {
            top: descriptionLabel.bottom
            topMargin: 5
            left: parent.left
            right: closeButton.left
            margins: 20
        }

        Timer {
            id: initialFocusTimer
            interval: 20
            onTriggered: () => {
                urlField.forceActiveFocus()
                urlField.selectAll()
            }
        }

        Component.onCompleted: initialFocusTimer.start()

        Keys.onPressed: keyEvent => {
            if ((keyEvent.modifiers & Qt.ControlModifier) && keyEvent.key === Qt.Key_Return) {
                saveButton.click()
            }
        }
    }

    Button {
        id: saveButton
        text: qsTr("Save")
        highlighted: true
        enabled: internal.isModified
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 20
        }

        onClicked: () => internal.commitChanges()
    }
}
