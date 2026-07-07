#pragma once

#include <QString>

struct CardDAVAddressBookFeederConfig
{
    QString baseNumber;
    QString host;
    QString path;
    QString user;
    int port;
    bool useSSL;
};
