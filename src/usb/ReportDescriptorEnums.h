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

        LED_OffHook = 0x0817,
        LED_Mute = 0x0809,
        LED_Ring = 0x0818,
        LED_Hold = 0x0820,
        LED_Microphone = 0x0821,

        Button_Primary = 0x0901,
    };
    Q_ENUM(UsageId)

    static QString toString(const UsageId id);
    static UsageId intToUsageId(const quint32 id);

    enum class UsageType { Unknown, Input, Output, Feature };
    Q_ENUM(UsageType)

    static QString toString(const UsageType id);
};
