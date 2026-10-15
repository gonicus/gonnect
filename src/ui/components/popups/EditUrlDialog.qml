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
        readonly property bool isModified: !!control.chatRoom && internal.trimmedUrl !== control.chatRoom.conferenceUrl

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
                                keyEvent.accepted = true
                                saveButton.click()
                            }
                        }
    }

    Label {
        id: noConnectorHintLabel
        visible: internal.trimmedUrl !== "" && !VideoCallHelper.hasMatchingConferenceConnector(internal.trimmedUrl)
        wrapMode: Label.Wrap
        color: Theme.secondaryTextColor
        text: qsTr("This URL does not belong to a configured conference server and cannot be joined from GOnnect.")
        anchors {
            top: urlField.bottom
            left: urlField.left
            right: urlField.right
            topMargin: Theme.d
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
