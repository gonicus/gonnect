pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 600
    implicitHeight: 800

    property IChatProvider chatProvider
    property string roomId
    property string messageId
    property string text

    Keys.onReturnPressed: () => internal.commitChanges()
    Keys.onEnterPressed:  () => internal.commitChanges()

    onTextChanged: () => contentTextField.text = control.text

    QtObject {
        id: internal

        readonly property string trimmedText: contentTextField.text.trim()
        readonly property bool isModified: internal.trimmedText !== control.text

        function commitChanges() {
            if (saveButton.enabled && internal.isModified) {
                if (internal.trimmedText) {
                    control.chatProvider.requestEditMessage(control.roomId, control.messageId, internal.trimmedText)
                } else {
                    control.chatProvider.requestRemoveMessage(control.roomId, control.messageId)
                }

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
        id: title
        text: qsTr("Edit text message")
        font.weight: Font.Medium
        elide: Label.ElideRight
        anchors {
            top: parent.top
            left: parent.left
            right: closeButton.left

            leftMargin: 20
            rightMargin: 20
        }
    }

    Flickable {
        id: textAreaFlickable
        clip: true
        contentHeight: contentTextField.implicitHeight
        anchors {
            top: title.bottom
            left: parent.left
            right: parent.right
            bottom: saveButton.top

            topMargin: 10
            leftMargin: 20
            rightMargin: 20
            bottomMargin: 10
        }

        ScrollBar.vertical: ScrollBar { width: 5 }

        TextArea {
            id: contentTextField
            wrapMode: TextEdit.Wrap
            anchors {
                left: parent.left
                right: parent.right
            }

            Timer {
                id: initialFocusTimer
                interval: 20
                onTriggered: () => {
                    contentTextField.forceActiveFocus()
                    contentTextField.selectAll()
                }
            }

            Component.onCompleted: initialFocusTimer.start()

            Keys.onPressed: keyEvent => {
                if ([Qt.Key_Return, Qt.Key_Enter].includes(keyEvent.key) && !(keyEvent.modifiers & Qt.ShiftModifier)) {
                    keyEvent.accepted = true
                    saveButton.click()
                }
            }
        }
    }

    Button {
        id: saveButton
        text: internal.trimmedText ? qsTr("Save") : qsTr("Remove")
        highlighted: true
        enabled: internal.isModified
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }

        onClicked: () => internal.commitChanges()
    }
}
