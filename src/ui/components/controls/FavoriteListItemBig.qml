pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: delg
    height: 54
    anchors {
        left: parent?.left
        right: parent?.right
    }

    required property string name
    required property string phoneNumber
    required property string company
    required property bool hasBuddyState
    required property bool hasAvatar
    required property string avatarPath
    required property int numberType
    required property int contactType

    readonly property bool isJitsiUrl: delg.contactType === NumberStats.ContactType.JitsiMeetUrl
    readonly property bool isReady: delg.buddyStatus === SIPBuddyState.READY
    property int buddyStatus: SIPBuddyState.UNKNOWN

    readonly property string typeIcon: {

        if (delg.isJitsiUrl) {
            return Icons.videoCall
        }

        switch (delg.numberType) {
            case Contact.NumberType.Commercial:
                return Icons.actor
            case Contact.NumberType.Mobile:
                return Icons.smartphone
            case Contact.NumberType.Home:
                return Icons.goHome
            default:
                return ''
        }
    }

    function updateBuddyStatus() {
        delg.buddyStatus = delg.hasBuddyState
                ? SIPManager.buddyStatus(delg.phoneNumber)
                : SIPBuddyState.UNKNOWN
    }

    function subscribeBuddyStatus() {
        const buddy = SIPManager.getBuddy(delg.phoneNumber)
        if (buddy !== null) {
            buddy.subscribeToBuddyStatus()
        }
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

    AvatarImage {
        id: avatarImage
        size: 40
        initials: ViewHelper.initials(delg.name || delg.phoneNumber)
        source: delg.hasAvatar ? ("file://" + delg.avatarPath) : ""
        showBuddyStatus: delg.hasBuddyState
        buddyStatus: delg.buddyStatus
        anchors {
            left: parent.left
            leftMargin: 10
            verticalCenter: parent.verticalCenter
        }
    }

    Item {
        id: nameCompanyContainer
        implicitHeight: contactNameLabel.implicitHeight
        implicitWidth: Math.max(contactNameLabel.implicitWidth, companyLabel.implicitWidth)
        anchors {
            left: avatarImage.right
            leftMargin: 10
            right: typeDelgLabel.left
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }

        Label {
            id: contactNameLabel
            text: delg.name || delg.phoneNumber
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

    IconLabel {
        id: typeDelgLabel
        icon.source: delg.typeIcon
        width: 20
        anchors {
            right: parent.right
            rightMargin: avatarImage.anchors.leftMargin
            verticalCenter: parent.verticalCenter
        }
    }

    Component {
        id: historyListContextMenuComponent

        HistoryListContextMenu {
            id: rowContextMenu
            phoneNumber: delg.phoneNumber
            isFavorite: true
            isSipSubscriptable: delg.hasBuddyState
            isReady: delg.isReady
            onCallClicked: () => SIPCallManager.call(delg.phoneNumber)
            onCallAsClicked: (identityId) => SIPCallManager.call("account0", delg.phoneNumber, "", identityId)
            onNotifyWhenAvailableClicked: () => delg.subscribeBuddyStatus()
        }
    }

    Component {
        id: jitsiHistoryListContextMenuComponent

        JitsiHistoryListContextMenu {
            id: rowJitsiContextMenu
            isFavorite: true
            roomName: delg.phoneNumber
            width: 230
            onCallClicked: () => {
                if (!ViewHelper.isActiveVideoCall) {
                    ViewHelper.requestMeeting(delg.phoneNumber)
                }
            }
        }
    }

    TapHandler {
        gesturePolicy: TapHandler.WithinBounds
        grabPermissions: PointerHandler.ApprovesTakeOverByAnything
        exclusiveSignals: TapHandler.SingleTap | TapHandler.DoubleTap
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onDoubleTapped: () => {
            if (delg.contactType === NumberStats.ContactType.JitsiMeetUrl) {
                if (!ViewHelper.isActiveVideoCall) {
                    ViewHelper.requestMeeting(delg.phoneNumber)
                }
            } else {
                SIPCallManager.call(delg.phoneNumber)
            }
        }
        onTapped: (_, mouseButton) => {
            if (mouseButton === Qt.RightButton) {
                if (delg.isJitsiUrl) {
                    jitsiHistoryListContextMenuComponent.createObject(delg).popup()
                } else {
                    historyListContextMenuComponent.createObject(delg).popup()
                }
            }
        }
    }

    HoverHandler {
        id: rowHoverHandler
    }
}
