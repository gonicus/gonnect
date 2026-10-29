#pragma once

#include "WebDavPropertyValue.h"

#include <QString>
#include <QHash>
#include <QDateTime>

struct WebDavItem
{
    QString href;
    int httpStatus = -1;
    WebDavPropertyValue property(const QString &namespaceUri, const QString &name) const;

    /// namespaceUri|propertyName --> WebDavPropertyValue
    QHash<QString, WebDavPropertyValue> properties;

    bool isCollection() const;
    bool isCalendarCollection() const;
    bool supportsComponent(const QString &componentName) const;

    QString name() const;
    QDateTime lastModified() const;
};
