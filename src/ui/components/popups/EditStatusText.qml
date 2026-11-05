pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 400
    implicitHeight: 260

    Keys.onReturnPressed: () => internal.commitChanges()
    Keys.onEnterPressed:  () => internal.commitChanges()

    QtObject {
        id: internal

        readonly property string trimmedText: contentTextField.text.trim()
        readonly property bool isModified: internal.trimmedText !== control.text

        function commitChanges() {
            if (saveButton.enabled && internal.isModified) {
                GlobalStateAggregator.statusText = internal.trimmedText
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

    TextArea {
        id: contentTextField
        placeholderText: qsTr("Your status message...")
        text: GlobalStateAggregator.statusText
        wrapMode: TextEdit.Wrap
        anchors {
            top: parent.top
            left: parent.left
            right: closeButton.left
            bottom: saveButton.top
            margins: 20
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
            if ((keyEvent.modifiers & Qt.ControlModifier) && keyEvent.key === Qt.Key_Return) {
                saveButton.click()
            }
        }
    }

    Button {
        id: saveButton
        text: internal.trimmedText ? qsTr("Set") : qsTr("Remove")
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
