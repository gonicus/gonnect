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

    property bool isCompactMode: false

    required property string name
    required property string company
    required property bool hasBuddyState
    required property bool hasAvatar
    required property string avatarPath
    required property var addresses
    required property string subscribableNumber

    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("Favorite contact")
    Accessible.description: qsTr("Selected favorite %1").arg(delg.name)
    Accessible.focusable: false

    states: [
        State {
            when: delg.isCompactMode

            PropertyChanges {
                addressButtonRow.visible: false
                buttonRowRepeater.model: null
                moreButton.visible: true
            }
            AnchorChanges {
                target: nameCompanyContainer
                anchors.right: moreButton.left
            }
        }
    ]

    QtObject {
        id: internal

        property int buddyStatus: SIPBuddyState.UNKNOWN

        function updateBuddyStatus() {
            internal.buddyStatus = delg.hasBuddyState
                    ? SIPManager.buddyStatus(delg.subscribableNumber)
                    : SIPBuddyState.UNKNOWN
        }

        function subscribeBuddyStatus() {
            const buddy = SIPManager.getBuddy(delg.subscribableNumber)
            if (buddy !== null) {
                buddy.subscribeToBuddyStatus()
            }
        }

        Component.onCompleted: () => internal.updateBuddyStatus()

        readonly property Connections sipManagerConnections: Connections {
            target: SIPManager
            enabled: delg.hasBuddyState
            function onBuddyStateChanged(url : string, status : int) {
                internal.updateBuddyStatus()
            }
        }

        function startMeetingOrCall(addr : var) {
            switch (addr.contactType) {
            case NumberStats.ContactType.JitsiMeetUrl:
                if (!ViewHelper.isActiveVideoCall) {
                    ViewHelper.requestMeeting(addr.addr)
                }
                break

            case NumberStats.ContactType.ChatRoomId:
                ViewHelper.showChatRoom(addr.chatProvider, addr.addr)
                break

            case NumberStats.ContactType.PhoneNumber:
                SIPCallManager.call(addr.addr)
                break
            }
        }

        function tooltipText(addr : var) : string {
            switch (addr.contactType) {
                case NumberStats.ContactType.JitsiMeetUrl:
                    return qsTr("Jitsi Meet (room '%1')").arg(addr.addr)

                case NumberStats.ContactType.ChatRoomId:
                    return qsTr("Chat with %1").arg(delg.name)

                case NumberStats.ContactType.PhoneNumber: {
                    switch (addr.numberType) {
                        case Contact.NumberType.Commercial:
                            return qsTr("Phone (Commercial, %1)").arg(addr.addr)

                        case Contact.NumberType.Mobile:
                            return qsTr("Phone (Mobile, %1)").arg(addr.addr)

                        case Contact.NumberType.Home:
                            return qsTr("Phone (Home, %1)").arg(addr.addr)
                    }
                }
            }
            return ''
        }

        function iconSource(addr : var) : string {
            switch (addr.contactType) {
                case NumberStats.ContactType.JitsiMeetUrl:
                   return Icons.videoCall

                case NumberStats.ContactType.ChatRoomId:
                   return Icons.dialogMessages

                case NumberStats.ContactType.PhoneNumber: {
                    switch (addr.numberType) {
                        case Contact.NumberType.Commercial:
                            return Icons.actor
                        case Contact.NumberType.Mobile:
                            return Icons.smartphone
                        case Contact.NumberType.Home:
                            return Icons.goHome
                    }
                }
            }
            return ''
        }
    }

    AvatarImage {
        id: avatarImage
        size: 40
        initials: ViewHelper.initials(delg.name)
        source: delg.hasAvatar
                ? (delg.avatarPath.startsWith("file://")
                   ? delg.avatarPath
                   : ("file://" + delg.avatarPath))
                : ""
        showPresenceStatus: delg.hasBuddyState
        presenceStatus: internal.buddyStatus
        indicatorComponent: Component { BuddyStatusIndicator {} }
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
            right: addressButtonRow.left
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }

        Label {
            id: contactNameLabel
            text: delg.name
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

            Accessible.ignored: true
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

            Accessible.ignored: true
        }

        Accessible.ignored: true
    }

    Item {
        id: moreButton
        visible: false
        width: avatarImage.size
        height: avatarImage.size
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        Rectangle {
            anchors.fill: parent
            radius: moreButton.width / 2
            color: moreButtonHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : 'transparent'
        }

        IconLabel {
            anchors.centerIn: parent
            icon {
                width: 24
                height: 24
                color: delg.enabled ? Theme.primaryTextColor : Theme.secondaryInactiveTextColor
                source: Icons.overflowMenu
            }
        }

        HoverHandler {
            id: moreButtonHoverHandler
        }

        TapHandler {
            onTapped: () => moreMenuComponent.createObject(moreButton).popup()
        }
    }

    Component {
        id: moreMenuComponent

        Menu {
            id: moreMenu
            width: {
                let maxWidth = 0
                let paddingSize = 0
                for (let i = 0; i < moreMenu.count; ++i) {
                    const item = moreMenu.itemAt(i)
                    if (item) {
                        maxWidth = Math.max(item.contentItem.implicitWidth, maxWidth)
                        paddingSize = Math.max(item.padding, paddingSize)
                    }
                }
                return maxWidth + (paddingSize * 2)
            }

            onClosed: () => moreMenu.destroy()

            Instantiator {
                model: delg.addresses
                delegate: MenuItem {
                    id: menuDelg
                    text: internal.tooltipText(menuDelg.modelData)
                    icon.source: internal.iconSource(menuDelg.modelData)

                    required property var modelData

                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Favorite phone, chat or meeting button")
                    Accessible.description: qsTr("Selected address %1").arg(menuDelg.modelData.addr)
                    Accessible.focusable: true
                    Accessible.onPressAction: () => internal.startMeetingOrCall(menuDelg.modelData)

                    // ToolTip.visible: addrHoverHandler.hovered
                    // ToolTip.text: internal.tooltipText(addrDelg.modelData)

                    onTriggered: () => internal.startMeetingOrCall(menuDelg.modelData)
                }

                onObjectAdded: (index, object) => moreMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => moreMenu.removeItem(object)
            }
        }
    }

    Row {
        id: addressButtonRow
        height: avatarImage.size
        anchors {
            right: parent.right
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }

        Repeater {
            id: buttonRowRepeater
            model: delg.addresses
            delegate: Item {
                id: addrDelg
                width: addrDelg.height
                anchors {
                    top: parent?.top
                    bottom: parent?.bottom
                }

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Favorite phone, chat or meeting button")
                Accessible.description: qsTr("Selected address %1").arg(addrDelg.modelData.addr)
                Accessible.focusable: true
                Accessible.onPressAction: () => internal.startMeetingOrCall(addrDelg.modelData)

                ToolTip.visible: addrHoverHandler.hovered
                ToolTip.text: internal.tooltipText(addrDelg.modelData)

                Rectangle {
                    anchors.fill: parent
                    radius: addrDelg.width / 2
                    color: addrHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : 'transparent'
                }

                IconLabel {
                    anchors.centerIn: parent
                    icon {
                        width: 24
                        height: 24
                        color: delg.enabled ? Theme.primaryTextColor : Theme.secondaryInactiveTextColor
                        source: internal.iconSource(addrDelg.modelData)
                    }
                }

                required property var modelData

                TapHandler {
                    gesturePolicy: TapHandler.WithinBounds
                    grabPermissions: PointerHandler.ApprovesTakeOverByAnything
                    exclusiveSignals: TapHandler.SingleTap
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onTapped: (_, mouseButton) => {
                        if (mouseButton === Qt.RightButton) {
                            switch (addrDelg.modelData.contactType) {
                            case NumberStats.ContactType.JitsiMeetUrl:
                                jitsiHistoryListContextMenuComponent.createObject(addrDelg, { addr: addrDelg.modelData }).popup()
                                break

                            case NumberStats.ContactType.ChatRoomId:
                                chatRoomContextMenuComponent.createObject(addrDelg, { addr: addrDelg.modelData }).popup()
                                break

                            case NumberStats.ContactType.PhoneNumber:
                                historyListContextMenuComponent.createObject(addrDelg, { addr: addrDelg.modelData }).popup()
                                break
                            }
                        } else {
                            internal.startMeetingOrCall(addrDelg.modelData)
                        }
                    }
                }

                HoverHandler {
                    id: addrHoverHandler
                }
            }
        }
    }

    Component {
        id: historyListContextMenuComponent

        HistoryListContextMenu {
            id: rowContextMenu
            phoneNumber: rowContextMenu.addr.addr
            isFavorite: true
            isSipSubscriptable: delg.hasBuddyState
            onCallClicked: () => SIPCallManager.call(rowContextMenu.addr.addr)
            onCallAsClicked: identityId => SIPCallManager.call("account0", rowContextMenu.addr.addr, "", identityId)
            onNotifyWhenAvailableClicked: () => internal.subscribeBuddyStatus()

            property var addr
        }
    }

    Component {
        id: jitsiHistoryListContextMenuComponent

        JitsiHistoryListContextMenu {
            id: rowJitsiContextMenu
            isFavorite: true
            roomName: rowJitsiContextMenu.addr.addr
            width: 230
            onCallClicked: () => {
                if (!ViewHelper.isActiveVideoCall) {
                    ViewHelper.requestMeeting(rowJitsiContextMenu.addr.addr)
                }
            }

            property var addr
        }
    }

    Component {
        id: chatRoomContextMenuComponent

        Menu {
            id: chatRoomMenu
            onClosed: () => chatRoomMenu.destroy()
            Action {
                id: favToggleAction
                text: qsTr('Remove favorite')
                onTriggered: () => addr.chatProvider.requestToggleRoomFavorite(addr.chatRoom)
            }
            property var addr
        }
    }
}
