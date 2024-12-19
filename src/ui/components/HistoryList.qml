pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import QtQuick.Layouts
import base

Item {
    id: control

    property alias limit: historyModel.limit
    property bool showScrollBar: false

    readonly property alias count: list.count
    readonly property bool hasPastCalls: list.count > 0
    readonly property HistoryProxyModel proxyModel: list.model

    Label {
        anchors.centerIn: parent
        visible: !control.hasPastCalls
        color: Theme.secondaryTextColor
        font.pixelSize: 18
        text: "🕓  " + qsTr("No past calls")
    }

    ListView {
        id: list
        clip: true
        topMargin: 20
        visible: control.hasPastCalls
        anchors.fill: parent

        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar
            visible: control.showScrollBar
            width: 10
            policy: ScrollBar.AlwaysOn
        }

        model: HistoryProxyModel {
            id: historyProxyModel

            HistoryModel {
                id: historyModel
            }
        }

        section.property: "day"
        section.delegate: Rectangle {
            id: sectionDelg
            radius: 4
            height: 25
            color: Theme.backgroundOffsetColor
            anchors {
                left: parent?.left
                right: parent?.right
                rightMargin: control.showScrollBar ? 20 : 0
            }

            required property date section

            Label {
                text: sectionDelg.section.toLocaleDateString(Qt.locale(), "dddd, dd. MMMM yyyy")
                anchors.centerIn: parent
            }
        }

        delegate: Item {
            id: delg

            height: 50
            anchors {
                left: parent?.left
                right: parent?.right
                rightMargin: control.showScrollBar ? 20 : 0
            }

            required property int id
            required property string contactId
            required property date time
            required property string contactName
            required property string company
            required property string location
            required property string account
            required property string remoteUrl
            required property string remotePhoneNumber
            required property int durationSeconds
            required property int type
            required property bool isBlocked
            required property bool isAnonymous
            required property bool isFavorite
            required property bool wasEstablished
            required property bool hasBuddyState
            required property bool hasAvatar
            required property string avatarPath

            property int buddyStatus: SIPBuddyState.UNKNOWN
            property bool isBusy: false

            function updateBuddyStatus() {
                delg.buddyStatus = delg.hasBuddyState ? SIPManager.buddyStatus(delg.remoteUrl) : SIPBuddyState.UNKNOWN
                delg.isBusy = (delg.buddyStatus === SIPBuddyState.BUSY)
            }

            Component.onCompleted: () => delg.updateBuddyStatus()

            Connections {
                target: SIPManager
                enabled: delg.hasBuddyState
                function onBuddyStateChanged(url : string, status : int) {
                    delg.updateBuddyStatus()
                }
            }

            Rectangle {
                id: rowBackground
                anchors.fill: parent
                radius: 4
                color: rowHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : 'transparent'
            }

            RowLayout {
                height: 40
                spacing: 0
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }

                AvatarImage {
                    id: avatarImage
                    initials: ViewHelper.initials(delg.contactName)
                    source: delg.hasAvatar ? ("file://" + delg.avatarPath) : ""
                    visible: delg.hasAvatar || delg.name !== ""
                    showBuddyStatus: delg.hasBuddyState || delg.isBlocked
                    buddyStatus: delg.buddyStatus
                    isBlocked: delg.isBlocked
                    size: 40

                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    id: nameCompanyContainer
                    Layout.preferredWidth: (delg.width - avatarImage.width - typeIcon.width - timesContainer.width - 50) / 2
                    Layout.alignment: Qt.AlignVCenter
                    implicitHeight: contactNameLabel.implicitHeight
                    implicitWidth: Math.max(contactNameLabel.implicitWidth, companyLabel.implicitWidth)

                    Label {
                        id: contactNameLabel
                        text: delg.contactName
                        font.weight: Font.Medium
                        elide: Label.ElideRight
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: !companyLabel.visible ? parent.verticalCenter : undefined
                            bottom: companyLabel.visible ? parent.verticalCenter : undefined
                        }
                    }

                    Label {
                        id: companyLabel
                        text: delg.company
                        visible: !!companyLabel.text
                        color: Theme.secondaryTextColor
                        elide: Label.ElideRight
                        anchors {
                            top: parent.verticalCenter
                            left: parent.left
                            right: parent.right
                        }
                    }
                }


                Item {
                    id: phoneNumberLocationContainer
                    Layout.preferredWidth: contactNameLabel.width
                    Layout.alignment: Qt.AlignVCenter
                    implicitHeight: contactNameLabel.implicitHeight
                    implicitWidth: Math.max(contactNameLabel.implicitWidth, companyLabel.implicitWidth)

                    Label {
                        id: phoneNumberLabel
                        text: delg.remotePhoneNumber
                        elide: Label.ElideRight
                        anchors {
                            left: parent.left
                            right: parent.right
                            bottom: locationLabel.visible ? parent.verticalCenter : undefined
                            verticalCenter: !locationLabel.visible ? parent.verticalCenter : undefined
                        }
                    }

                    Label {
                        id: locationLabel
                        visible: locationLabel.text !== ""
                        color: Theme.secondaryTextColor
                        elide: Label.ElideRight
                        text: delg.location
                        anchors {
                            top: parent.verticalCenter
                            left: parent.left
                            right: parent.right
                        }
                    }
                }

                Item {
                    id: typeIcon
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    Layout.rightMargin: 10

                    IconLabel {
                        height: 20
                        width: 20
                        anchors.centerIn: parent
                        icon {
                            width: 20
                            height: 20
                            source: delg.type === CallHistoryItem.Type.IncomingBlocked
                                    ? Icons.dialogCancel
                                    : (delg.type === CallHistoryItem.Type.Incoming
                                       ? (delg.wasEstablished ? Icons.callIncoming : Icons.callMissed)
                                       : Icons.callOutgoing)
                        }
                    }
                }

                Item {
                    id: timesContainer
                    implicitHeight: timeLabel.implicitHeight
                    Layout.preferredWidth: 60
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                    Item {
                        id: timeLabel
                        implicitHeight: timeTextLabel.implicitHeight
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: !durationLabel.visible ? parent.verticalCenter : undefined
                            bottom: durationLabel.visible ? parent.verticalCenter : undefined
                        }

                        Label {
                            id: timeIconLabel
                            width: 15
                            text: "🕓"
                            horizontalAlignment: Label.AlignHCenter
                            anchors {
                                left: parent.left
                                verticalCenter: parent.verticalCenter
                                verticalCenterOffset: 2
                            }
                        }

                        Label {
                            id: timeTextLabel
                            text: Qt.formatDateTime(delg.time, qsTr("hh:mm"))
                            anchors {
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }
                        }
                    }

                    Item {
                        id: durationLabel
                        implicitHeight: durationTextLabel.implicitHeight
                        visible: delg.wasEstablished
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.verticalCenter
                        }

                        Label {
                            text: "⌛"
                            width: timeIconLabel.width
                            horizontalAlignment: Label.AlignHCenter
                            anchors {
                                left: parent.left
                                verticalCenter: parent.verticalCenter
                            }
                        }

                        Label {
                            id: durationTextLabel
                            text: ViewHelper.secondsToNiceText(delg.durationSeconds)
                            anchors {
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }

            function subscribeBuddyStatus() {
                const buddy = SIPManager.getBuddy(delg.remoteUrl)
                if (buddy !== null) {
                    buddy.subscribeToBuddyStatus()
                }
            }

            HistoryListContextMenu {
                id: rowContextMenu
                phoneNumber: delg.remotePhoneNumber
                isFavorite: delg.isFavorite
                isAnonymous: delg.isAnonymous
                isBusy: delg.isBusy
                isBlocked: delg.isBlocked
                width: 230
                onCallClicked: () => SIPCallManager.call(delg.account, delg.remoteUrl, delg.contactId)
                onNotifyWhenAvailableClicked: () => delg.subscribeBuddyStatus()
                onBlockTemporarilyClicked: () => SIPCallManager.toggleTemporaryBlock(delg.contactId, delg.remotePhoneNumber)
            }

            TapHandler {
                enabled: !delg.isAnonymous
                gesturePolicy: TapHandler.WithinBounds
                grabPermissions: PointerHandler.ApprovesTakeOverByAnything
                exclusiveSignals: TapHandler.SingleTap | TapHandler.DoubleTap
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onDoubleTapped: () => SIPCallManager.call(delg.account, delg.remoteUrl, delg.contactId)
                onTapped: (_, mouseButton) => {
                    if (mouseButton === Qt.RightButton) {
                        rowContextMenu.popup()
                    }
                }
            }

            HoverHandler {
                id: rowHoverHandler
                enabled: !delg.isAnonymous
            }
        }
    }
}
