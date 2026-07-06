pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Item {
    id: control
    implicitWidth: 600
    implicitHeight: 800

    property IChatProvider chatProvider
    property string roomId


    Keys.onReturnPressed: () => internal.saveRoom()
    Keys.onEnterPressed:  () => internal.saveRoom()

    QtObject {
        id: internal

        readonly property IChatRoom room: control.chatProvider?.chatRoomByRoomId(control.roomId) ?? null
        readonly property string originalRoomName: internal.room?.name ?? ""
        readonly property bool originalIsDirectRoom: internal.room?.isDirectChat ?? false
        readonly property int originalJoinRule: internal.room?.joinRule ?? IChatRoom.JoinRule.Unknown
        readonly property string originalAvatarPath: internal.room?.avatarPath

        readonly property bool isModified: roomNameTextField.text.trim() !== internal.originalRoomName
                                           || internal.originalJoinRule !== joinRuleComboBox.currentValue
                                           || internal.originalAvatarPath !== avatarImg.source

        onOriginalRoomNameChanged: () => Qt.callLater(() => {
                                                          roomNameTextField.text = internal.originalRoomName
                                                          avatarImg.initials = ViewHelper.initials(internal.originalRoomName)
                                                      })
        onOriginalJoinRuleChanged: () => internal.preselectJoinRule()
        onOriginalAvatarPathChanged: () => avatarImg.source = internal.originalAvatarPath
        onRoomChanged: () => internal.preselectJoinRule()

        function preselectJoinRule() {
            joinRuleComboBox.currentValue = internal.originalJoinRule
        }

        function saveRoom() {
            if (createRoomButton.enabled) {
                control.chatProvider.requestRoomChange(internal.room,
                                                       roomNameTextField.text.trim(),
                                                       joinRuleComboBox.currentValue,
                                                       avatarImg.source)

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

        onClicked: () => control.StackView.view.popCurrentItem(StackView.Immediate)
    }

    RoomAvatar {
        id: avatarImg
        chatProvider: control.chatProvider
        anchors {
            top: parent.top
            left: parent.left
            margins: 10
        }
    }

    TextField {
        id: roomNameTextField
        placeholderText: qsTr("Room name")
        anchors {
            verticalCenter: avatarImg.verticalCenter
            left: avatarImg.right
            right: closeButton.left
            margins: 20
        }

        onTextChanged: () => avatarImg.initials = ViewHelper.initials(roomNameTextField.text.trim())

        Timer {
            id: initialFocusTimer
            interval: 20
            onTriggered: () => {
                roomNameTextField.forceActiveFocus()
                roomNameTextField.selectAll()
            }
        }

        Component.onCompleted: initialFocusTimer.start()
    }

    Item {
        id: joinRuleRow
        implicitHeight: joinRuleComboBox.implicitHeight
        visible: !internal.originalIsDirectRoom
        anchors {
            top: roomNameTextField.bottom
            left: parent.left
            right: parent.right
            margins: 20
        }

        Label {
            id: joinRuleLabel
            text: qsTr("Join rule:")
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
            }
        }

        JoinRuleComboBox {
            id: joinRuleComboBox
            anchors {
                left: joinRuleLabel.right
                right: parent.right
                margins: 20
            }
        }
    }

    Button {
        id: createRoomButton
        text: qsTr("Save")
        highlighted: true
        enabled: internal.isModified
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 20
        }

        onClicked: () => internal.saveRoom()
    }
}
