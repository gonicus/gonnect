pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

ListView {
    id: control
    topMargin: 20
    clip: true
    model: FavoritesModel {}
    header: Rectangle {
        id: headerItem
        radius: 4
        height: 25
        color: Theme.backgroundOffsetColor
        anchors {
            left: parent?.left
            right: parent?.right
        }

        Label {
            text: qsTr("Favorites")
            elide: Label.ElideRight
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
                margins: 10
            }
        }
    }

    delegate: Item {
        id: delg
        height: 30
        anchors {
            left: parent?.left
            right: parent?.right
        }

        required property string contactId
        required property string name
        required property string phoneNumber
        required property bool hasBuddyState
        required property bool hasAvatar
        required property string avatarPath
        required property int numberType
        required property bool isAnonymous
        required property bool isBlocked

        readonly property string typeIcon: {
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

        property int buddyStatus: SIPBuddyState.UNKNOWN
        property bool isBusy: false

        function updateBuddyStatus() {
            delg.buddyStatus = delg.hasBuddyState
                    ? SIPManager.buddyStatus(delg.phoneNumber)
                    : SIPBuddyState.UNKNOWN
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

        AvatarImage {
            id: avatarImage
            initials: ViewHelper.initials(delg.name || delg.phoneNumber)
            source: delg.hasAvatar ? ("file://" + delg.avatarPath) : ""
            showBuddyStatus: delg.hasBuddyState || delg.isBlocked
            buddyStatus: delg.buddyStatus
            isBlocked: delg.isBlocked
            anchors {
                left: parent.left
                verticalCenter: parent.verticalCenter
            }
        }

        Label {
            id: nameLabel
            elide: Label.ElideRight
            text: delg.name || delg.phoneNumber
            anchors {
                left: avatarImage.right
                leftMargin: 10
                right: typeDelgLabel.left
                rightMargin: 10
                verticalCenter: parent.verticalCenter
            }
        }

        IconLabel {
            id: typeDelgLabel
            icon.source: delg.typeIcon
            width: 16
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
        }

        function subscribeBuddyStatus() {
            const buddy = SIPManager.getBuddy(delg.phoneNumber)
            if (buddy !== null) {
                buddy.subscribeToBuddyStatus()
            }
        }

        HistoryListContextMenu {
            id: rowContextMenu
            phoneNumber: delg.phoneNumber
            isFavorite: true
            isAnonymous: delg.isAnonymous
            isBusy: delg.isBusy
            isBlocked: delg.isBlocked
            width: 230
            onCallClicked: () => SIPCallManager.call(delg.phoneNumber)
            onNotifyWhenAvailableClicked: () => delg.subscribeBuddyStatus()
            onBlockTemporarilyClicked: () => SIPCallManager.toggleTemporaryBlock(delg.contactId, delg.phoneNumber)
        }

        TapHandler {
            gesturePolicy: TapHandler.WithinBounds
            grabPermissions: PointerHandler.ApprovesTakeOverByAnything
            exclusiveSignals: TapHandler.SingleTap | TapHandler.DoubleTap
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onDoubleTapped: () => SIPCallManager.call(delg.phoneNumber)
            onTapped: (_, mouseButton) => {
                if (mouseButton === Qt.RightButton) {
                    rowContextMenu.popup()
                }
            }
        }

        HoverHandler {
            id: rowHoverHandler
        }
    }
}
