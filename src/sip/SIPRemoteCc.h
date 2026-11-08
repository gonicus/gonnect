#pragma once
#include <QString>

class SIPAccount;

class SIPRemoteCc
{
public:
    static bool registerModule();
    static bool sendSoftKeyEvent(SIPAccount *account, const QString &softKeyEvent);
};
