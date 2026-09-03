pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitHeight: 64
    visible: control.shallBeVisible || control.isLoadingMessageHistory

    property IChatProvider chatProvider
    property IChatRoom chatRoom
    property bool shallBeVisible

    readonly property bool isLoadingMessageHistory: control.chatRoom?.isLoadingMessageHistory ?? false
    readonly property Contact soleOtherContact: control.chatRoom && control.chatRoom.isDirectChat
                                                ? ContactHelper.lookupByChatUser(control.chatRoom.otherUser)
                                                : null

    readonly property PhoneNumbersModel numbersModel: PhoneNumbersModel {
        contact: control.soleOtherContact
    }

    AvatarImage {
        id: avatarImage
        size: Theme.d * 4
        source: control.chatRoom?.avatarPath ?? ""
        initials: control.chatRoom ? ViewHelper.initials(control.chatRoom.name) : ""
        showPresenceStatus: !!(control.chatRoom?.hasPresenceState)
        presenceStatus: control.chatRoom?.presenceState ?? ChatUser.PresenceState.Unknown
        indicatorComponent: Component { ChatUserPresenceStatusIndicator {} }
        anchors {
            left: parent.left
            leftMargin: Theme.d
            verticalCenter: parent.verticalCenter
        }
    }

    Label {
        id: mainLabel
        font.pixelSize: 16
        font.weight: Font.Medium
        elide: Text.ElideRight
        color: Theme.secondaryTextColor
        text: control.chatRoom
              ? (control.chatRoom.isDirectChat
                 ? qsTr("Direct conversation with %1").arg(control.chatRoom.name)
                 : qsTr("Chat room %1").arg(control.chatRoom.name))
              : ""
        anchors {
            left: avatarImage.visible ? avatarImage.right : parent.left
            leftMargin: Theme.d
            verticalCenter: parent.verticalCenter
            right: rightPart.left
        }
    }

    Row {
        id: rightPart
        spacing: Math.floor(Theme.d / 2)
        rightPadding: Theme.d * 2
        leftPadding: Theme.d * 2
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }

        Row {
            id: titleLoadingIndicatorRow
            spacing: Math.floor(Theme.d / 2)
            rightPadding: Theme.d * 2
            leftPadding: Theme.d * 2
            visible: control.isLoadingMessageHistory
            anchors {
                top: parent.top
                bottom: parent.bottom
            }

            BusyIndicator {
                id: titleLoadingIndicator
                running: titleLoadingIndicatorRow.visible
                width: titleLoadingIndicator.height
                height: 24
                circleColor: Theme.secondaryTextColor
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                text: qsTr("Messages are loading...")
                color: Theme.secondaryTextColor
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        BottomButtonBarSeparator {}

        BarButton {
            id: favButton
            iconPath: Icons.folderFavorites
            toggled: control.chatRoom?.isFavorite ?? false
            text: qsTr("Favorite")
            onClicked: () => control.chatProvider?.requestToggleRoomFavorite(control.chatRoom)
        }

        BarButton {
            id: optionsButton
            iconPath: Icons.settingsConfigure
            showDropdownButton: true
            text: qsTr("Options")
            onClicked: () => {
                           chatRoomMenuComponent.createObject(optionsButton, {
                                                                  toggleFavoriteVisible: false,
                                                                  editRoomVisible: !!(Number(control.chatRoom?.permissions ?? 0) & IChatRoom.Permission.CanEdit),
                                                                  inviteUsersVisible: !!(Number(control.chatRoom?.permissions ?? 0) & IChatRoom.Permission.CanInvite)
                                                              }).popup()
                       }
        }

        Button {
            id: callButton
            width: 50
            height: 50
            highlighted: true
            visible: !!control.soleOtherContact
            icon.source: Icons.callStart
            anchors.verticalCenter: parent.verticalCenter

            Material.accent: Theme.greenColor

            Component.onCompleted: () => {
                callButton.icon.width = 24
                callButton.icon.height = 24
            }

            onClicked: () => {
                           const soleNumber = control.numbersModel.soleNumber()
                           if (soleNumber !== "") {
                               SIPCallManager.call(soleNumber)
                           } else {
                               const item = phoneNumbersMenuComponent.createObject(callButton, { contact: control.soleOtherContact })
                               if (!item) {
                                   console.error("Error on creating phone numbers menu")
                               }
                               item.popup()
                               item.updateWidth()
                           }
                       }

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Accept call")
            Accessible.focusable: true
            Accessible.onPressAction: () => callButton.click()
        }
    }

    Component {
        id: chatRoomMenuComponent

        ChatRoomContextMenu {
            onEditRoomTriggered: () => ViewHelper.showEditRoomDialog(control.chatProvider, control.chatRoom.id)
            onInviteUsersTriggered: () => ViewHelper.showInviteUserToRoomDialog(control.chatProvider, control.chatRoom.id)
            onLeaveRoomTriggered: () => {
                                      const item = DialogFactory.createConfirmDialog({
                                                       text: qsTr("Are you sure you really want to leave this chat?")
                                                   })
                                      const roomId = control.chatRoom.id
                                      const chatProvider = control.chatProvider
                                      item.accepted.connect(() => chatProvider.requestRoomLeave(roomId))
                                  }
        }
    }

    Component {
        id: phoneNumbersMenuComponent

        Menu {
            id: phoneNumberMenu
            onClosed: () => phoneNumberMenu.destroy()

            required property Contact contact

            function updateWidth() {
                let w = 0
                for (let i = 0, l = phoneNumberMenu.count; i < l; ++i) {
                    const item = phoneNumberMenu.itemAt(i)
                    w = Math.max(w, item.contentItem.implicitWidth + item.padding * 2)
                }
                phoneNumberMenu.width = w
            }

            Instantiator {
                model: control.numbersModel
                delegate: MenuItem {
                    id: menuDelg
                    text: PhoneNumberUtil.tooltipText(menuDelg.addr, phoneNumberMenu.contact?.computedName ?? "")
                    icon.source: PhoneNumberUtil.iconSource(menuDelg.addr)

                    required property string number
                    required property int type

                    readonly property var addr: ({
                                                     addr: menuDelg.number,
                                                     numberType: menuDelg.type,
                                                     contactType: NumberStats.ContactType.PhoneNumber
                                                 })

                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Call contact button")
                    Accessible.description: qsTr("Selected number %1").arg(menuDelg.number)
                    Accessible.focusable: true
                    Accessible.onPressAction: () => PhoneNumberUtil.startMeetingOrCall(menuDelg.addr)

                    onTriggered: () => SIPCallManager.call(menuDelg.number)
                }

                onObjectAdded: (index, object) => {
                                   phoneNumberMenu.insertItem(index, object)
                                   phoneNumberMenu.updateWidth()
                               }
                onObjectRemoved: (index, object) => {
                                     phoneNumberMenu.removeItem(object)
                                     phoneNumberMenu.updateWidth()
                                 }
            }
        }
    }
}
