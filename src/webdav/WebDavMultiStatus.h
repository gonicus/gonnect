#pragma once

#include "WebDavItem.h"

#include <QString>
#include <QList>

struct WebDavMultiStatus
{
    QList<WebDavItem> items;
    QString syncToken;
};
