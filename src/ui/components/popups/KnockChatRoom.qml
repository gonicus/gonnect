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

    Keys.onReturnPressed: () => internal.knockRoom()
    Keys.onEnterPressed:  () => internal.knockRoom()

    QtObject {
        id: internal

        function knockRoom() {
            control.chatProvider.knockRoomRequest(roomId, messageArea.text.trim())
            control.StackView.view.popCurrentItem(StackView.Immediate)
        }
    }

    HeaderIconButton {
        id: closeButton
        iconSource: Icons.mobileCloseApp
        anchors {
            top: parent.top
            right: parent.right
        }

        onClicked: () => control.StackView.view.popCurrentItem(StackView.Immediate)
    }

    Label {
        id: descriptionLabel
        wrapMode: Text.Wrap
        color: Theme.secondaryTextColor
        text: qsTr("The room cannot be joined directly, but you can issue a request to become a member of it. Any user eligible to accept the request will be informed about it - with a message you can optionally enter below.")
        anchors {
            top: parent.top
            left: parent.left
            right: closeButton.left
            margins: 20
        }
    }

    TextArea {
        id: messageArea
        placeholderText: qsTr("Optional message")
        anchors {
            top: descriptionLabel.bottom
            left: parent.left
            right: parent.right
            bottom: knockButton.top
            margins: 20
        }

        Timer {
            id: initialFocusTimer
            interval: 20
            onTriggered: () => messageArea.forceActiveFocus()

        }

        Component.onCompleted: initialFocusTimer.start()
    }

    Button {
        id: knockButton
        text: qsTr("Request")
        highlighted: true
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 20
        }

        onClicked: () => internal.knockRoom()
    }
}
