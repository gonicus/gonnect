#pragma once

#include "WebDavPropertyValue.h"

#include <QString>
#include <QHash>

struct WebDavItem
{
    QString href;
    int httpStatus = -1;
    WebDavPropertyValue property(const QString &namespaceUri, const QString &name) const;

    /// namespaceUri|propertyName --> WebDavPropertyValue
    QHash<QString, WebDavPropertyValue> properties;

    bool isCollection() const;
};
