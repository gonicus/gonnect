pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 600
    implicitHeight: 800

    property IChatProvider chatProvider
    property alias userIds: proxyModel.selectedUserIds
    property alias roomName: roomNameTextField.text

    Keys.onReturnPressed: () => internal.createRoom()
    Keys.onEnterPressed:  () => internal.createRoom()

    QtObject {
        id: internal

        readonly property ChatUser directUser: {
            if (singleRoomRadioButton.checked && proxyModel.selectedUserIds.length === 1) {
                const userId = proxyModel.selectedUserIds[0]
                return control.chatProvider.userById(userId)
            }
            return null
        }

        readonly property string directUserName: internal.directUser?.computedName ?? ""

        function createRoom() {
            if (createRoomButton.enabled) {
                if (singleRoomRadioButton.checked) {
                    if (control.userIds.length !== 1) {
                        throw new Error("Expecting exactly one user id but got " + control.userIds.length)
                    }

                    control.chatProvider.requestDirectRoomCreation(control.userIds[0],
                                                                   roomNameTextField.text.trim(),
                                                                   avatarImg.source)
                } else {
                    control.chatProvider.requestGroupRoomCreation(control.userIds,
                                                                  joinRuleComboBox.currentValue,
                                                                  roomNameTextField.text.trim(),
                                                                  avatarImg.source)
                }

                control.StackView.view.popCurrentItem(StackView.Immediate)
            }
        }

        function preselectRoomType() {
            if (singleRoomRadioButton.enabled && control.userIds.length === 1) {
                singleRoomRadioButton.checked = true
            } else {
                groupRoomRadioButton.checked = true
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
        initials: ViewHelper.initials(roomNameTextField.text.trim() || internal.directUserName) || ' '
        anchors {
            top: parent.top
            left: parent.left
            margins: 10
        }
    }

    TextField {
        id: roomNameTextField
        placeholderText: internal.directUserName || qsTr("Room name")
        anchors {
            verticalCenter: avatarImg.verticalCenter
            left: avatarImg.right
            right: closeButton.left
            margins: 20
        }

        Timer {
            id: initialFocusTimer
            interval: 20
            onTriggered: () => {
                roomNameTextField.forceActiveFocus()
                internal.preselectRoomType()
            }
        }

        Component.onCompleted: initialFocusTimer.start()
    }

    Row {
        id: radioButtonRow
        height: singleRoomRadioButton.implicitHeight
        spacing: 20
        anchors {
            top: roomNameTextField.bottom
            left: parent.left
            right: parent.right
            margins: 20
        }

        RadioButton {
            id: singleRoomRadioButton
            text: qsTr("Direct chat")
            enabled: control.userIds.length === 1

            onEnabledChanged: internal.preselectRoomType()
        }

        RadioButton {
            id: groupRoomRadioButton
            text: qsTr("Group chat")
        }
    }

    Item {
        id: joinRuleRow
        implicitHeight: joinRuleComboBox.implicitHeight
        visible: groupRoomRadioButton.checked
        anchors {
            top: radioButtonRow.bottom
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

    Label {
        id: inviteeListHeadingLabel
        wrapMode: Text.Wrap
        color: Theme.secondaryTextColor
        text: qsTr("These users will be invited to the chat:")
        anchors {
            top: joinRuleComboBox.visible ? joinRuleRow.bottom : radioButtonRow.bottom
            left: parent.left
            right: parent.right
            margins: 20
        }
    }

    SearchField {
        id: searchField
        placeHolderText: qsTr("Search user...")
        anchors {
            left: parent.left
            right: parent.right
            top: inviteeListHeadingLabel.bottom
            margins: 20
        }
    }

    ListView {
        id: inviteeList
        clip: true
        model: ChatUsersProxyModel {
            id: proxyModel
            filterText: searchField.text

            ChatUsersModel {
                chatProvider: control.chatProvider
            }
        }

        anchors {
            left: parent.left
            right: parent.right
            rightMargin: 20
            topMargin: 20
            top: searchField.bottom
            bottom: createRoomButton.top
        }
        delegate: Item {
            id: delg
            height: 50
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property string id
            required property string name
            required property string avatarPath
            required property bool hasPresenceState
            required property int presenceState

            CheckBox {
                id: selectedBox
                checked: proxyModel.selectedUserIds.includes(delg.id)
                anchors {
                    left: parent.left
                    leftMargin: 20
                    verticalCenter: parent.verticalCenter
                }

                onToggled: () => {
                    proxyModel.toggleSelectedState(delg.id)
                }
            }

            AvatarImage {
                id: userAvatarImg
                initials: ViewHelper.initials(delg.name)
                source: delg.avatarPath
                size: 40
                showPresenceStatus: delg.hasPresenceState
                presenceStatus: delg.presenceState
                indicatorComponent: Component { ChatUserPresenceStatusIndicator {} }
                anchors {
                    left: selectedBox.right
                    leftMargin: 10
                    verticalCenter: parent.verticalCenter
                }
            }

            Label {
                text: delg.name
                elide: Text.ElideRight
                anchors {
                    left: userAvatarImg.right
                    leftMargin: 10
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    Button {
        id: createRoomButton
        text: qsTr("Create")
        highlighted: true
        enabled: singleRoomRadioButton.checked || groupRoomRadioButton.checked
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 20
        }

        onClicked: () => internal.createRoom()
    }
}
