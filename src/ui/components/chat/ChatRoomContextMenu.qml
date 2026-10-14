pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Menu {
    id: control
    onClosed: () => control.destroy()

    signal favoriteToggled
    signal leaveRoomTriggered
    signal editRoomTriggered
    signal inviteUsersTriggered
    signal editConferenceUrlTriggered
    signal indexingToggled

    property bool toggleFavoriteVisible: true
    property bool editRoomVisible: true
    property bool inviteUsersVisible: true
    property bool editConferenceUrlVisible: false
    property bool toggleIndexingVisible: false
    property bool indexingEnabled: false

    property string conferenceUrl: ""
    property Contact contact: null

    HideableMenuItem {
        id: conferenceMenuItem
        visible: control.conferenceUrl !== ""
        text: qsTr("Join conference")
        icon.source: Icons.videoCall
        onTriggered: () => VideoCallHelper.joinOrStartConference(control.conferenceUrl)
    }

    HideableMenuSeparator {
        visible: conferenceMenuItem.visible
    }

    HideableMenuSeparator {
        visible: numbersModel.count > 0
    }

    HideableMenuItem {
        visible: control.toggleFavoriteVisible
        text: qsTr("Toggle favorite")
        icon.source: Icons.folderFavorites
        onTriggered: () => control.favoriteToggled()
    }

    HideableMenuItem {
        visible: control.editRoomVisible
        text: qsTr("Edit room...")
        icon.source: Icons.editor
        onTriggered: () => control.editRoomTriggered()
    }

    HideableMenuItem {
        visible: control.inviteUsersVisible
        text: qsTr("Invite users...")
        icon.source: Icons.listAdd
        onTriggered: () => control.inviteUsersTriggered()
    }

    HideableMenuItem {
        visible: control.editConferenceUrlVisible
        text: qsTr("Edit conference url...")
        icon.source: Icons.videoCall
        onTriggered: () => control.editConferenceUrlTriggered()
    }

    HideableMenuItem {
        text: qsTr("Leave room...")
        icon.source: Icons.dialogCancel
        onTriggered: () => control.leaveRoomTriggered()
    }

    HideableMenuItem {
        text: control.indexingEnabled ? qsTr("Disable search index") : qsTr("Enable search index")
        icon.source: Icons.systemSearch
        onTriggered: () => control.indexingToggled()
    }

    Instantiator {
        model: PhoneNumbersModel {
            id: numbersModel
            contact: control.contact
        }
        delegate: MenuItem {
            id: menuDelg
            text: PhoneNumberUtil.tooltipText(menuDelg.addr, control.contact?.computedName ?? "")
            icon.source: PhoneNumberUtil.iconSource(menuDelg.addr)

            required property string number
            required property int type

            readonly property var addr: ({
                                             addr: menuDelg.number,
                                             numberType: menuDelg.type,
                                             contactType: NumberStats.ContactType.PhoneNumber
                                         })

            onTriggered: () => SIPCallManager.call(menuDelg.number)
        }

        onObjectAdded: (index, object) => control.insertItem(2 + index, object)
        onObjectRemoved: (index, object) => control.removeItem(object)
    }
}
