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
        onCountChanged: () => internal.updateHasActiveCall()
    }

    onSoleOtherContactChanged: () => internal.updateHasActiveCall()

    QtObject {
        id: internal

        readonly property IConferenceConnector activeConferenceConnector: control.chatRoom && control.chatRoom.conferenceUrl
                                                                          ? VideoCallHelper.matchingConferenceConnector(control.chatRoom.conferenceUrl)
                                                                          : null

            property bool hasActiveCall

        Component.onCompleted: () => internal.updateHasActiveCall()

        readonly property Connections sipCallManagerConnections: Connections {
            target: SIPCallManager
            function onCallsChanged() { internal.updateHasActiveCall() }
            function onCallContactChanged() { internal.updateHasActiveCall() }
        }

        function updateHasActiveCall() {
            const contact = control.soleOtherContact
            internal.hasActiveCall = !!contact && SIPCallManager.hasCallWithContact(contact)
        }
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
        font.pixelSize: Theme.fontSizeMedium
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
        leftPadding: Theme.d
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }

        Row {
            id: titleLoadingIndicatorRow
            spacing: Math.floor(Theme.d / 2)
            rightPadding: Math.floor(Theme.d / 2)
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

        ButtonBarSeparator {
            visible: titleLoadingIndicatorRow.visible
        }

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
            text: qsTr("More")
            showDropdownButton: true
            onDropDownClicked: () => optionsButton.clicked()
            onClicked: () => {
                           chatRoomMenuComponent.createObject(optionsButton, {
                                                                  toggleFavoriteVisible: false,
                                                                  editRoomVisible: !!(Number(control.chatRoom?.permissions ?? 0) & IChatRoom.Permission.CanEdit),
                                                                  inviteUsersVisible: !!(Number(control.chatRoom?.permissions ?? 0) & IChatRoom.Permission.CanInvite),
                                                                  editConferenceUrlVisible: !!(Number(control.chatRoom?.permissions ?? 0) & IChatRoom.Permission.CanEditConferenceUrl)
                                                              }).popup()
                       }
        }

        BarButton {
            id: startConferenceButton
            text: qsTr("Conference")
            toggled: true
            toggledColor: Theme.greenColor
            iconPath: Icons.videoCall
            enabled: !VideoCallHelper.hasActiveVideoCall
            visible: !!internal.activeConferenceConnector && !leaveConferenceButton.visible

            Accessible.name: qsTr("Start conference")

            onClicked: () => VideoCallHelper.joinOrStartConfernece(control.chatRoom?.conferenceUrl)
        }

        BarButton {
            id: leaveConferenceButton
            text: qsTr("Leave")
            toggled: true
            toggledColor: Theme.redColor
            iconPath: Icons.callStop
            visible: !!internal.activeConferenceConnector && control.chatRoom.conferenceUrl === VideoCallHelper.activeVideoCall

            Accessible.name: qsTr("Leave conference")

            onClicked: () => {
                const conn = internal.activeConferenceConnector

                if (conn.ownRole === ConferenceUser.Role.Moderator && conn.numberOfUsers > 1) {
                    leaveMenu.popup(leaveConferenceButton, -leaveMenu.width + leaveConferenceButton.width, leaveConferenceButton.height)
                } else {
                    internal.activeConferenceConnector.leaveConference()
                }
            }

            Menu {
                id: leaveMenu

                MenuItem {
                    text: qsTr("Leave conference")
                    onClicked: () => {
                        internal.activeConferenceConnector.leaveConference()
                    }
                }

                MenuItem {
                    text: qsTr("End conference for all")
                    onClicked: () => {
                        internal.activeConferenceConnector.terminateConference()
                    }
                }
            }
        }

        BarButton {
            id: callButton
            text: qsTr("Call")
            toggled: true
            toggledColor: Theme.greenColor
            iconPath: Icons.callStart
            visible: !!control.soleOtherContact && !internal.hasActiveCall

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

            Accessible.name: qsTr("Start phone call")
        }

        BarButton {
            id: hangupButton
            text: qsTr("Hang up")
            toggled: true
            toggledColor: Theme.redColor
            iconPath: Icons.callStop
            visible: !!control.soleOtherContact && internal.hasActiveCall

            onClicked: () => {
                           if (SIPCallManager.isConferenceMode) {
                               SIPCallManager.endConference()
                           } else if (control.soleOtherContact) {
                               SIPCallManager.endCallWithContact(control.soleOtherContact)
                           } else {
                               console.error("Cannot hang up due to missing phone number")
                           }
                       }

            Accessible.name: qsTr("Hang up phone call")
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
            onEditConferenceUrlTriggered: () => ViewHelper.requestUrlEditDialog(control.chatRoom)
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
