#pragma once
#include <QObject>

class ReportDescriptorEnums
{
    Q_GADGET

public:
    enum class UsageId {

        Unknown = 0x0000,

        Telephony_Headset = 0x0B05,
        Telephony_TelephonyKeyPad = 0x0B06,
        Telephony_ProgrammableButton = 0x0B07,
        Telephony_HookSwitch = 0x0B20,
        Telephony_Flash = 0x0B21,
        Telephony_Redial = 0x0B24,
        Telephony_PhoneMute = 0x0B2F,
        Telephony_SpeedDial = 0x0B50,
        Telephony_LineBusyTone = 0x0B97,
        Telephony_Ringer = 0x0B9E,

        Teams_UCDISPLAY = 0x0001,
        Teams_VendorExtension = 0xFF00,
        Teams_DisplayAttributes = 0x0020,
        Teams_DisplayControl = 0x0024,
        Teams_CharacterAttributes = 0x0048,
        Teams_CharacterReport = 0x002B,
        Teams_IconsControl = 0xFF17,
        Teams_Button = 0x0004,
        Teams_ASPNotification = 0x0003,

        LED_OffHook = 0x0817,
        LED_Mute = 0x0809,
        LED_Ring = 0x0818,
        LED_Hold = 0x0820,
        LED_Microphone = 0x0821,

        Button_Primary = 0x0901,

        Vendor_LEDCommand = 0xFF01,
    };
    Q_ENUM(UsageId)

    enum class TeamsDisplayFieldSupport {
        LocalUserName = 1,
        LocalUserStatus = 2,
        Date = 3,
        Time = 4,
        CallStatus = 5,
        OtherPartyName = 6,
        OtherPartyTitle = 7,
        Subject = 8,
        Duration = 9,
        Number = 10,
        OtherPartyNumber = 11,
        ConversationID = 12,
    };
    Q_ENUM(TeamsDisplayFieldSupport)

    enum class TeamsScreenSelect {
        NoChange = 0,
        HomeScreen = 1,
        ReadyToCall = 2,
        OutgoingCall = 3,
        IncomingCall = 4,
        InCall = 5,
        HoldCall = 6,
        EndCall = 7,
    };
    Q_ENUM(TeamsScreenSelect)

    enum class TeamsPresenceIcon {
        NotSet = 0,
        Online = 1,
        IDLE = 2,
        Busy = 3,
        BusyIDLE = 4,
        AWAY = 5,
        DND = 6,
        Offline = 7,
        OfflineIDLE = 8,
    };
    Q_ENUM(TeamsPresenceIcon)

    static QString toString(const UsageId id);
    static UsageId intToUsageId(const quint32 id);

    enum class UsageType { Unknown, Input, Output, Feature };
    Q_ENUM(UsageType)

    static QString toString(const UsageType id);
    static QString toString(const TeamsDisplayFieldSupport id);
    static QString toString(const TeamsScreenSelect id);
    static QString toString(const TeamsPresenceIcon id);
};
