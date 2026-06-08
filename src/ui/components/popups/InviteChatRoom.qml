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

    Keys.onReturnPressed: () => internal.inviteUsers()
    Keys.onEnterPressed:  () => internal.inviteUsers()

    QtObject {
        id: internal

        readonly property IChatRoom room: control.chatProvider?.chatRoomByRoomId(control.roomId) ?? null

        function inviteUsers() {
            if (inviteUsersButton.enabled) {
                control.chatProvider.inviteUsers(control.roomId, proxyModel.selectedUserIds)
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

    Label {
        id: inviteeListHeadingLabel
        wrapMode: Text.Wrap
        color: Theme.secondaryTextColor
        text: qsTr('Select the users that shall be invited to chat room "%1". Those who already are users are excluded from the list.').arg(internal.room?.name ?? "")
        anchors {
            top: parent.top
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

        Component.onCompleted: () => focusTimer.start()

        Timer {
            id: focusTimer
            interval: 20
            onTriggered: searchField.giveFocus()
        }
    }

    ListView {
        id: inviteeList
        clip: true
        model: ChatUsersProxyModel {
            id: proxyModel
            filterText: searchField.text
            excludedUserIds: internal.room?.chatUsers.map(user => user.id)

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
            bottom: inviteUsersButton.top
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
                id: avatarImg
                initials: ViewHelper.initials(delg.name)
                source: delg.avatarPath
                size: 40
                showPresenceStatus: delg.hasPresenceState
                presenceStatus: delg.presenceState
                indicatorComponent: Component { ChatUserPresenceStatusIndicator {} }
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: selectedBox.right
                    leftMargin: 10
                }
            }

            Label {
                text: delg.name
                elide: Text.ElideRight
                anchors {
                    left: avatarImg.right
                    leftMargin: 10
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    Button {
        id: inviteUsersButton
        text: qsTr("Invite")
        highlighted: true
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 20
        }

        onClicked: () => internal.inviteUsers()
    }
}
