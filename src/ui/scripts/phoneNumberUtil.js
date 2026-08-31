.pragma library

.import base as B

function droggelbecher() {
    return `Droggelbecher: ${B.NumberStats.ContactType.JitsiMeetUrl} ${B.Contact.NumberType.Mobile}`
}

function startMeetingOrCall(addr) {
    switch (addr.contactType) {
    case B.NumberStats.ContactType.JitsiMeetUrl:
        if (!B.ViewHelper.isActiveVideoCall) {
            B.ViewHelper.requestMeeting(addr.addr)
        }
        break

    case B.NumberStats.ContactType.ChatRoomId:
        B.ViewHelper.showChatRoom(addr.chatProvider, addr.addr)
        break

    case B.NumberStats.ContactType.PhoneNumber:
        B.SIPCallManager.call(addr.addr)
        break
    }
}

function tooltipText(addr, displayName) {
    switch (addr.contactType) {
        case B.NumberStats.ContactType.JitsiMeetUrl:
            return qsTr("Jitsi Meet (room '%1')").arg(addr.addr)

        case B.NumberStats.ContactType.ChatRoomId:
            return qsTr("Chat with %1").arg(displayName)

        case B.NumberStats.ContactType.PhoneNumber: {
            switch (addr.numberType) {
                case B.Contact.NumberType.Commercial:
                    return qsTr("Phone (Commercial, %1)").arg(addr.addr)

                case B.Contact.NumberType.Mobile:
                    return qsTr("Phone (Mobile, %1)").arg(addr.addr)

                case B.Contact.NumberType.Home:
                    return qsTr("Phone (Home, %1)").arg(addr.addr)

                case B.Contact.NumberType.Unknown:
                    return qsTr("Phone (%1)").arg(addr.addr)
            }
        }
    }
    return ''
}

function iconSource(addr) {

    switch (addr.contactType) {
        case B.NumberStats.ContactType.JitsiMeetUrl:
           return B.Icons.videoCall

        case B.NumberStats.ContactType.ChatRoomId:
           return B.Icons.dialogMessages

        case B.NumberStats.ContactType.PhoneNumber: {
            switch (addr.numberType) {
                case B.Contact.NumberType.Commercial:
                    return B.Icons.actor

                case B.Contact.NumberType.Mobile:
                    return B.Icons.smartphone

                case B.Contact.NumberType.Home:
                    return B.Icons.goHome

                case B.Contact.NumberType.Unknown:
                    return B.Icons.callStart
            }
            console.error("Number type", addr.numberType, "could not be matched")
        }
    }

    console.error("Contact type", addr.contactType, "could not be matched")
    return ''
}
