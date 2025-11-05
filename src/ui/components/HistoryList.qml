pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import QtQuick.Layouts
import base

Item {
    id: control

    property alias limit: historyModel.limit
    property int listMargin: 20

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
            width: 10
            clip: true

            // TODO: This is *still* broken...
            property double scrollBarRadius: 50

            contentItem: Rectangle {
                color: "#7d8182"
                bottomRightRadius: {
                    let isAtBottom = (verticalScrollBar.position + verticalScrollBar.size) >= 0.98

                    if (isAtBottom) {
                        return verticalScrollBar.scrollBarRadius
                    } else {
                        return 0
                    }
                }
            }

            background: Rectangle {
                color: Theme.backgroundOffsetColor
                bottomRightRadius: verticalScrollBar.scrollBarRadius
            }
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

                leftMargin: control.listMargin
                rightMargin: control.listMargin
            }

            required property date section

            Label {
                text: sectionDelg.section.toLocaleDateString(Qt.locale(), "dddd, dd. MMMM yyyy")
                anchors.centerIn: parent
            }
        }

        delegate: Item {
            id: delg
            enabled: ViewHelper.isJitsiAvailable || !delg.isJitsiMeetCall
            height: 50
            anchors {
                left: parent?.left
                right: parent?.right

                leftMargin: control.listMargin
                rightMargin: control.listMargin
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
            readonly property bool isReady: delg.buddyStatus === SIPBuddyState.READY

            readonly property bool isJitsiMeetCall: delg.type & CallHistoryItem.Type.JitsiMeetCall
            readonly property bool isSIPCall: delg.type & CallHistoryItem.Type.SIPCall

            function updateBuddyStatus() {
                delg.buddyStatus = delg.hasBuddyState ? SIPManager.buddyStatus(delg.remoteUrl) : SIPBuddyState.UNKNOWN
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
                            verticalCenter: parent.verticalCenter
                        }
                        states: [
                            State {
                                when: companyLabel.visible
                                AnchorChanges {
                                    target: contactNameLabel
                                    anchors {
                                        verticalCenter: undefined
                                        bottom: contactNameLabel.parent?.verticalCenter
                                    }
                                }
                            }
                        ]
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
                    Layout.preferredWidth: nameCompanyContainer.Layout.preferredWidth
                    Layout.alignment: Qt.AlignVCenter
                    implicitHeight: phoneNumberLabel.implicitHeight
                    implicitWidth: Math.max(phoneNumberLabel.implicitWidth, locationLabel.implicitWidth)

                    Label {
                        id: phoneNumberLabel
                        elide: Label.ElideRight
                        text: delg.remotePhoneNumber
                        anchors {
                            left: parent.left
                            right: parent.right
                            bottom: parent.verticalCenter
                        }

                        states: [
                            State {
                                when: !locationLabel.visible
                                AnchorChanges {
                                    target: phoneNumberLabel
                                    anchors {
                                        bottom: undefined
                                        verticalCenter: phoneNumberLocationContainer.verticalCenter
                                    }
                                }
                            }
                        ]
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
                            source: {
                                if (delg.type & CallHistoryItem.Type.JitsiMeetCall && !(delg.type & CallHistoryItem.Type.SIPCall)) {
                                    return Icons.videoCall
                                }
                                if (delg.type & CallHistoryItem.Type.IncomingBlocked) {
                                    return Icons.dialogCancel
                                }
                                if (delg.type & CallHistoryItem.Type.Outgoing) {
                                    return Icons.callOutgoing
                                }
                                if (delg.wasEstablished) {
                                    return Icons.callIncoming
                                }
                                return Icons.callMissed
                            }
                        }
                    }
                }

                Item {
                    id: timesContainer
                    implicitHeight: timeLabel.implicitHeight
                    Layout.preferredWidth: 70
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                    Item {
                        id: timeLabel
                        implicitHeight: timeTextLabel.implicitHeight
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        states: [
                            State {
                                when: durationLabel.visible
                                AnchorChanges {
                                    target: timeLabel
                                    anchors {
                                        verticalCenter: undefined
                                        bottom: timeLabel.parent?.verticalCenter
                                    }
                                }
                            }
                        ]

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

            Component {
                id: historyListContextMenuComponent

                HistoryListContextMenu {
                    id: rowContextMenu
                    phoneNumber: delg.remotePhoneNumber
                    isFavorite: delg.isFavorite
                    isAnonymous: delg.isAnonymous
                    isBlocked: delg.isBlocked
                    isSipSubscriptable: delg.hasBuddyState
                    isReady: delg.isReady
                    width: 230
                    onCallClicked: () => SIPCallManager.call(delg.account, delg.remoteUrl, delg.contactId)
                    onCallAsClicked: (identityId) => SIPCallManager.call(delg.account, delg.remoteUrl, delg.contactId, identityId)
                    onNotifyWhenAvailableClicked: () => delg.subscribeBuddyStatus()
                    onBlockTemporarilyClicked: () => SIPCallManager.toggleTemporaryBlock(delg.contactId, delg.remotePhoneNumber)
                }
            }

            Component {
                id: jitsiHistoryListContextMenuComponent

                JitsiHistoryListContextMenu {
                    id: rowJitsiContextMenu
                    isFavorite: delg.isFavorite
                    roomName: delg.remotePhoneNumber
                    width: 230
                    onCallClicked: () => ViewHelper.requestMeeting(delg.remoteUrl)
                }
            }

            TapHandler {
                enabled: !delg.isAnonymous
                gesturePolicy: TapHandler.WithinBounds
                grabPermissions: PointerHandler.ApprovesTakeOverByAnything
                exclusiveSignals: TapHandler.SingleTap | TapHandler.DoubleTap
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onDoubleTapped: () => {
                    if (delg.isSIPCall) {
                        SIPCallManager.call(delg.account, delg.remoteUrl, delg.contactId)
                    } else if (delg.isJitsiMeetCall && !ViewHelper.isActiveVideoCall) {
                        ViewHelper.requestMeeting(delg.remoteUrl)
                    }
                }
                onTapped: (_, mouseButton) => {
                    if (mouseButton === Qt.RightButton) {
                        if (delg.isSIPCall) {
                            historyListContextMenuComponent.createObject(delg).popup()
                        } else if (delg.isJitsiMeetCall) {
                            jitsiHistoryListContextMenuComponent.createObject(delg).popup()
                        }
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
