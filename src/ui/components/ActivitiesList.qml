pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import QtQuick.Layouts
import base

Item {
    id: control

    property alias limit: activitiesModel.limit
    property int listMargin: 20

    readonly property alias count: list.count
    readonly property bool hasActivities: list.count > 0

    Label {
        anchors.centerIn: parent
        visible: !control.hasActivities
        color: Theme.secondaryTextColor
        font.pixelSize: 18
        text: "🕒  " + qsTr("No activities")

        Accessible.role: Accessible.StaticText
        Accessible.name: qsTr("No activities")
    }

    ListView {
        id: list
        clip: true
        topMargin: 20
        visible: control.hasActivities
        anchors.fill: parent

        Accessible.role: Accessible.List
        Accessible.name: qsTr("Activities")
        Accessible.description: qsTr("List of recent calls, meetings and chat messages")

        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar
            width: 10
            clip: true
        }

        model: ActivitiesModel {
            id: activitiesModel
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
                id: sectionLabel
                text: sectionDelg.section.toLocaleDateString(Qt.locale(), "dddd, dd. MMMM yyyy")
                anchors.centerIn: parent
            }

            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("Activities item section")
            Accessible.description: qsTr("Header for the currently selected day: %1").arg(sectionLabel.text)
        }

        delegate: Item {
            id: delg
            enabled: ViewHelper.isJitsiAvailable || !delg.isJitsiMeetCall || delg.isSIPCall
            height: 50
            anchors {
                left: parent?.left
                right: parent?.right

                leftMargin: control.listMargin
                rightMargin: control.listMargin
            }

            required property int id
            required property bool isSIPCall
            required property bool isJitsiMeetCall
            required property bool isChatMessage
            required property date time
            required property string title
            required property string subtitle
            required property string text
            required property string location
            required property bool hasAvatar
            required property string avatarPath
            required property string account
            required property string contactId
            required property string remoteUrl
            required property string remotePhoneNumber
            required property int durationSeconds
            required property bool wasEstablished
            required property bool isAnonymous
            required property bool isFavorite
            required property bool isBlocked
            required property bool hasBuddyState
            required property list<string> hops
            required property int callType
            required property string chatProviderId
            required property string chatRoomId
            required property bool isOwnMessage

            property int buddyStatus: SIPBuddyState.UNKNOWN
            readonly property bool isReady: delg.buddyStatus === SIPBuddyState.READY

            // Subtitle of chat entries shows the sender, which does not apply to the own user.
            readonly property string displaySubtitle: delg.isChatMessage && delg.isOwnMessage
                                                      ? qsTr("Me") : delg.subtitle

            function updateBuddyStatus() {
                delg.buddyStatus = delg.hasBuddyState ? SIPManager.buddyStatus(delg.remoteUrl)
                                                      : SIPBuddyState.UNKNOWN
            }

            function openChatRoom() {
                for (const provider of ChatConnectorManager.chatConnectors) {
                    if (provider.id === delg.chatProviderId) {
                        ViewHelper.showChatRoom(provider, delg.chatRoomId)
                        return
                    }
                }
            }

            function removeEntry() {
                const id = delg.id
                const item = DialogFactory.createConfirmDialog({
                                 text: qsTr("Are you sure you really want to remove this entry?")
                             })
                item.accepted.connect(() => activitiesModel.removeEntry(id))
            }

            Component.onCompleted: () => delg.updateBuddyStatus()

            Accessible.role: Accessible.ListItem
            Accessible.name: qsTr("Activities item")
            Accessible.description: qsTr("Selected activity %1 - %2 - time %3").arg(delg.title).arg(delg.text).arg(timeTextLabel.text)
            Accessible.focusable: true

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
                    initials: ViewHelper.initials(delg.isChatMessage ? delg.displaySubtitle : delg.title)
                    source: delg.hasAvatar ? ("file://" + delg.avatarPath) : ""
                    visible: delg.hasAvatar || delg.title !== ""
                    showPresenceStatus: !delg.isChatMessage && (delg.hasBuddyState || delg.isBlocked)
                    presenceStatus: delg.buddyStatus
                    isBlocked: delg.isBlocked
                    size: 40
                    indicatorComponent: Component { BuddyStatusIndicator {} }

                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    id: nameContainer
                    Layout.preferredWidth: (delg.width - avatarImage.width - typeIcon.width - timesContainer.width - 50) / 2
                    Layout.alignment: Qt.AlignVCenter
                    implicitHeight: titleLabel.implicitHeight
                    implicitWidth: Math.max(titleLabel.implicitWidth, subtitleLabel.implicitWidth)

                    Label {
                        id: titleLabel
                        text: delg.title
                        font.weight: Font.Medium
                        elide: Label.ElideRight
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        states: [
                            State {
                                when: subtitleLabel.visible
                                AnchorChanges {
                                    target: titleLabel
                                    anchors {
                                        verticalCenter: undefined
                                        bottom: titleLabel.parent?.verticalCenter
                                    }
                                }
                            }
                        ]

                        Accessible.ignored: true
                    }

                    Label {
                        id: subtitleLabel
                        text: delg.displaySubtitle
                        visible: subtitleLabel.text !== ""
                        color: Theme.secondaryTextColor
                        elide: Label.ElideRight
                        anchors {
                            top: parent.verticalCenter
                            left: parent.left
                            right: parent.right
                        }

                        Accessible.ignored: true
                    }
                }

                Item {
                    id: textContainer
                    Layout.preferredWidth: nameContainer.Layout.preferredWidth
                    Layout.alignment: Qt.AlignVCenter
                    implicitHeight: textLabel.implicitHeight
                    implicitWidth: Math.max(textLabel.implicitWidth, locationLabel.implicitWidth)

                    Label {
                        id: textLabel
                        elide: Label.ElideRight
                        text: delg.text + (delg.hops.length > 0
                                           ? qsTr(", via %1").arg(delg.hops.join(" → "))
                                           : "")
                        anchors {
                            left: parent.left
                            right: parent.right
                            bottom: parent.verticalCenter
                        }

                        states: [
                            State {
                                when: !locationLabel.visible
                                AnchorChanges {
                                    target: textLabel
                                    anchors {
                                        bottom: undefined
                                        verticalCenter: textContainer.verticalCenter
                                    }
                                }
                            }
                        ]

                        Accessible.ignored: true
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

                        Accessible.ignored: true
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
                                if (delg.isChatMessage) {
                                    return Icons.dialogMessages
                                }
                                if (delg.callType & CallHistoryItem.Type.JitsiMeetCall && !(delg.callType & CallHistoryItem.Type.SIPCall)) {
                                    return Icons.videoCall
                                }
                                if (delg.callType & CallHistoryItem.Type.IncomingBlocked) {
                                    return Icons.dialogCancel
                                }
                                if (delg.callType & CallHistoryItem.Type.Outgoing) {
                                    return Icons.callOutgoing
                                }
                                if (delg.wasEstablished) {
                                    return Icons.callIncoming
                                }
                                return Icons.callMissed
                            }
                        }
                    }

                    Accessible.ignored: true
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

                        IconLabel {
                            id: timeIconLabel
                            icon {
                                source: Icons.acceptTimeEvent
                                width: 18
                                height: 18
                            }
                            anchors {
                                left: parent.left
                                verticalCenter: parent.verticalCenter
                                verticalCenterOffset: 2
                            }

                            Accessible.ignored: true
                        }

                        Label {
                            id: timeTextLabel
                            text: Qt.formatDateTime(delg.time, qsTr("hh:mm"))
                            anchors {
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }

                            Accessible.ignored: true
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

                        IconLabel {
                            icon {
                                source: Icons.chronometer
                                width: 18
                                height: 18
                            }
                            anchors {
                                left: parent.left
                                verticalCenter: parent.verticalCenter
                            }

                            Accessible.ignored: true
                        }

                        Label {
                            id: durationTextLabel
                            text: ViewHelper.secondsToNiceText(delg.durationSeconds)
                            anchors {
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }

                            Accessible.ignored: true
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
                    onRemoveItem: () => delg.removeEntry()
                }
            }

            Component {
                id: jitsiHistoryListContextMenuComponent

                JitsiHistoryListContextMenu {
                    isFavorite: delg.isFavorite
                    roomName: delg.remotePhoneNumber
                    width: 230
                    onCallClicked: () => ViewHelper.requestMeeting(delg.remoteUrl)
                    onRemoveItem: () => delg.removeEntry()
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
                    } else if (mouseButton === Qt.LeftButton && delg.isChatMessage) {
                        delg.openChatRoom()
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
