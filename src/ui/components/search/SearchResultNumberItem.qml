pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Rectangle {
    id: control
    implicitHeight: numberDelgLabel.height
    radius: 2
    color: control.highlighted ? Theme.highlightColor : 'transparent'

    property bool highlighted

    required property int type
    required property string number
    required property bool isSipStatusSubscriptable
    required property bool isFavorite
    required property string contactId

    readonly property string typeIcon: {
        switch (control.type) {
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
        control.buddyStatus = control.isSipStatusSubscriptable
                ? SIPManager.buddyStatus(control.number)
                : SIPBuddyState.UNKNOWN
    }

    signal manuallyHovered
    signal triggerPrimaryAction
    signal triggerSecondaryAction

    Component.onCompleted: () => control.updateBuddyStatus()

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Phone number")
    Accessible.description: qsTr("Selected phone number: ") +
                            qsTr("Number: ") + control.number +
                            (control.isFavorite ? qsTr("Hint: This is a favorite") : "")
    Accessible.focusable: true
    Accessible.onPressAction: () => control.triggerPrimaryAction()

    Connections {
        target: SIPManager
        enabled: control.isSipStatusSubscriptable
        function onBuddyStateChanged(url : string, status : int) {
            control.updateBuddyStatus()
        }
    }

    IconLabel {
        id: typeDelgLabel
        visible: !control.isSipStatusSubscriptable
        width: 16
        icon.source: control.typeIcon
        anchors {
            left: parent.left
            leftMargin: 17
            verticalCenter: parent.verticalCenter
        }
    }

    BuddyStatusIndicator {
        id: buddyStatusIndicator
        visible: control.isSipStatusSubscriptable
        status: control.buddyStatus
        anchors {
            left: parent.left
            leftMargin: 20
            verticalCenter: parent.verticalCenter
        }
    }

    Label {
        id: numberDelgLabel
        text: control.number
        elide: Label.ElideMiddle
        anchors {
            leftMargin: 40
            left: parent.left
            right: favIcon.left
            rightMargin: 1
        }

        Accessible.ignored: true
    }

    FavIcon {
        id: favIcon
        visible: true
        isFavorite: control.isFavorite
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -2
        }

        TapHandler {
            grabPermissions: PointerHandler.TakeOverForbidden
            gesturePolicy: TapHandler.WithinBounds
            onTapped: () => {
                ViewHelper.toggleFavorite(control.number, NumberStats.ContactType.PhoneNumber)
            }
        }
    }

    HoverHandler {
        id: hoverHandler
        cursorShape: Qt.PointingHandCursor
        onHoveredChanged: () => {
            if (hoverHandler.hovered) {
                control.manuallyHovered()
            }
        }
    }

    TapHandler {
        gesturePolicy: TapHandler.WithinBounds
        grabPermissions: PointerHandler.ApprovesTakeOverByAnything
        exclusiveSignals: TapHandler.SingleTap
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onTapped: (_, mouseButton) => {
            if (mouseButton === Qt.RightButton) {
                control.triggerSecondaryAction()
            } else {
                control.triggerPrimaryAction()
            }
        }
    }
}
