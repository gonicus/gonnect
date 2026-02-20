pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property alias conferenceConnector: participantsModel.conferenceConnector

    readonly property alias count: participantListView.count

    ListView {
        id: participantListView
        anchors.fill: parent
        topMargin: 10
        bottomMargin: 10
        model: ParticipantsModel {
            id: participantsModel
        }

        Accessible.role: Accessible.List
        Accessible.name: qsTr("Participants list")
        Accessible.description: qsTr("List of all the participants of the current chat room")

        delegate: Item {
            id: delg
            height: 54
            anchors {
                left: parent?.left
                right: parent?.right
            }

            required property string id
            required property string displayName
            required property int role

            readonly property bool isModerator: delg.role === ConferenceParticipant.Role.Moderator
            readonly property bool isMe: delg.id === control.conferenceConnector?.ownId ?? false

            Accessible.role: Accessible.ListItem
            Accessible.name: qsTr("Chat participant")
            Accessible.description: qsTr("Selected chat participant: ") +
                                    qsTr("Name: ") + delg.displayName +
                                    (delg.isModerator ? qsTr("Role: Moderator") : "") +
                                    (delg.isMe ? qsTr("Hint: This is your account") : "")
            Accessible.focusable: true

            Rectangle {
                id: selectedBackground
                visible: control.conferenceConnector.largeVideoParticipant?.id === delg.id
                color: Theme.backgroundOffsetHoveredColor
                radius: 4
                anchors {
                    fill: parent
                    leftMargin: 10
                    rightMargin: 10
                }
            }

            Rectangle {
                id: hoverBackground
                visible: participantHoverHandler.hovered
                color: Theme.backgroundOffsetHoveredColor
                radius: 4
                anchors {
                    fill: parent
                    leftMargin: 10
                    rightMargin: 10
                }
            }

            AvatarImage {
                id: avatarImage
                initials: ViewHelper.initials(delg.displayName)
                size: 40
                anchors {
                    left: parent.left
                    leftMargin: 20
                    verticalCenter: parent.verticalCenter
                }
            }

            Label {
                id: adminCrown
                text: "👑"
                rotation: 20
                visible: delg.isModerator
                font.pixelSize: 24
                anchors {
                    horizontalCenter: avatarImage.horizontalCenter
                    horizontalCenterOffset: 9
                    verticalCenter: avatarImage.top
                }

                Accessible.ignored: true
            }

            Label {
                id: nameLabel
                text: delg.displayName
                wrapMode: Label.WordWrap
                maximumLineCount: 2
                elide: Label.ElideRight
                font.weight: Font.Medium
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: avatarImage.right
                    right: parent.right
                    leftMargin: 10
                    rightMargin: 20
                }

                Accessible.ignored: true
            }

            HoverHandler {
                id: participantHoverHandler
            }

            TapHandler {
                gesturePolicy: TapHandler.WithinBounds
                grabPermissions: PointerHandler.ApprovesTakeOverByAnything
                exclusiveSignals: TapHandler.SingleTap
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onTapped: (_, mouseButton) => {
                    if (mouseButton === Qt.RightButton) {
                        // The functions features in the context menu are not working right now (jitsi meet issues), so hide
                        // the menu completely until fixed.

                        if (!delg.isMe && delg.isModerator) {
                            participantContextMenuComponent.createObject(delg).popup()
                        }
                    } else {
                        if (control.conferenceConnector.largeVideoParticipant?.id === delg.id) {
                            control.conferenceConnector.setLargeVideoParticipantById("")
                        } else {
                            control.conferenceConnector.setLargeVideoParticipantById(delg.id)
                        }
                    }
                }
            }

            Component {
                id: participantContextMenuComponent

                Menu {
                    id: participantContextMenu

                    MenuItem {
                        id: participantKick
                        text: qsTr("Kick")
                        onClicked: () => control.conferenceConnector.kickParticipant(delg.id)

                        Accessible.role: Accessible.MenuItem
                        Accessible.name: participantKick.text
                        Accessible.focusable: true
                        Accessible.onPressAction: () => participantKick.click()
                    }
                    MenuItem {
                        id: participantMakeMod
                        enabled: !delg.isModerator
                        text: qsTr("Make moderator")
                        onClicked: () => control.conferenceConnector.grantParticipantRole(delg.id, ConferenceParticipant.Role.Moderator)

                        Accessible.role: Accessible.MenuItem
                        Accessible.name: participantKick.text
                        Accessible.focusable: true
                        Accessible.onPressAction: () => participantMakeMod.click()
                    }
                }
            }
        }
    }
}
