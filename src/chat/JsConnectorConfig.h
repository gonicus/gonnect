#pragma once

#include <QString>
#include <QUrl>

struct JsConnectorConfig
{
    QString settingsGroup;
    QUrl url;
    QString ownUserId;
    QString deviceId;
    QString displayName;
    QString recoveryKey;
    QString accessToken;
};
