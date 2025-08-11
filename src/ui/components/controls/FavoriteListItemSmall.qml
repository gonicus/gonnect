pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: delg
    height: 30
    anchors {
        left: parent?.left
        right: parent?.right
    }

    required property string name
    required property string phoneNumber
    required property bool hasBuddyState
    required property bool hasAvatar
    required property string avatarPath
    required property int numberType

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

    function updateBuddyStatus() {
        delg.buddyStatus = delg.hasBuddyState
                ? SIPManager.buddyStatus(delg.phoneNumber)
                : SIPBuddyState.UNKNOWN
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
        showBuddyStatus: delg.hasBuddyState
        buddyStatus: delg.buddyStatus
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
    }

    IconLabel {
        id: typeDelgLabel
        icon.source: delg.typeIcon
        width: 16
        anchors {
            left: avatarImage.right
            leftMargin: 7
            verticalCenter: parent.verticalCenter
        }
    }

    Label {
        id: nameLabel
        elide: Label.ElideRight
        text: delg.name || delg.phoneNumber
        anchors {
            left: avatarImage.right
            leftMargin: 30
            right: favIcon.left
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }
    }

    FavIcon {
        id: favIcon
        isFavorite: true
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        TapHandler {
            grabPermissions: PointerHandler.TakeOverForbidden
            gesturePolicy: TapHandler.WithinBounds
            onTapped: () => {
                ViewHelper.toggleFavorite(delg.phoneNumber)
            }
        }
    }

    TapHandler {
        gesturePolicy: TapHandler.WithinBounds
        grabPermissions: PointerHandler.ApprovesTakeOverByAnything
        exclusiveSignals: TapHandler.SingleTap
        onSingleTapped: () => SIPCallManager.call(delg.phoneNumber)
    }

    HoverHandler {
        id: rowHoverHandler
    }
}
