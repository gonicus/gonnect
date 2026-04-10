#pragma once

#include <QString>

struct CardDAVAddressBookFeederConfig
{
    QString host;
    QString path;
    QString user;
    int port;
    bool useSSL;
};
