pragma ComponentBehavior: Bound

import QtQuick

import base

QtObject {
    id: control

    required property string name
    required property string phoneNumber
    required property bool hasBuddyState
    required property int numberType

    readonly property string numberString: {
        switch (control.numberType) {
            case Contact.NumberType.Commercial:
            return qsTr('Commercial')
            case Contact.NumberType.Mobile:
            return qsTr('Mobile')
            case Contact.NumberType.Home:
            return qsTr('Home')
            default:
            return control.phoneNumber
        }
    }

    readonly property string text: control.name
          ? `${control.stateChar}${control.name} (${control.numberString})`
          : `${control.stateChar}${control.numberString}`


    readonly property string stateChar: {
        if (control.hasBuddyState) {
            switch (control.buddyStatus) {
                case SIPBuddyState.UNKNOWN:
                case SIPBuddyState.UNAVAILABLE:
                    return "⚪  "
                case SIPBuddyState.BUSY:
                    return "🔴  "
                default:
                    return "🟢  "
            }
        }
        return "⚫  "
    }

    property int buddyStatus: SIPBuddyState.UNKNOWN

    function updateBuddyStatus() {
        control.buddyStatus = control.hasBuddyState
                ? SIPManager.buddyStatus(control.phoneNumber)
                : SIPBuddyState.UNKNOWN
    }

    Component.onCompleted: () => control.updateBuddyStatus()

    readonly property Connections sipManagerConnections: Connections {
        target: SIPManager
        enabled: control.hasBuddyState
        function onBuddyStateChanged(url : string, status : int) {
            control.updateBuddyStatus()
        }
    }
}
