#pragma once

#include <QDateTime>
#include <QString>
#include <QUrl>

struct CalDAVEventFeederConfig
{
    QString settingsGroupId;
    QString source;
    QString host;
    QString path;
    QString user;
    int port;
    bool useSSL;
    int interval;
    QDateTime timeRangeStart;
    QDateTime timeRangeEnd;
};
