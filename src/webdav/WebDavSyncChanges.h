#pragma once

#include "WebDavSyncResult.h"

#include <QHash>
#include <QSet>
#include <QString>

struct WebDavSyncChanges
{
    WebDavSyncResult result = WebDavSyncResult::Success;

    // Normalized path -> raw href
    QHash<QString, QString> changedHrefs;

    // Normalized paths of removed members
    QSet<QString> removedPaths;
};
