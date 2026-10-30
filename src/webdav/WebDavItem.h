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
    bool hasProperty(const QString &namespaceUri, const QString &name) const;

    /// namespaceUri|propertyName --> WebDavPropertyValue
    QHash<QString, WebDavPropertyValue> properties;

    bool isCollection() const;
    bool isCalendarCollection() const;
    bool isContactCollection() const;
    bool supportsComponent(const QString &componentName) const;

    QString name() const;
    QDateTime lastModified() const;
    QString syncToken() const;

    // Canonical path/url
    QString normalizedPath() const;
    static QString normalizedPath(const QUrl &url);

    static QString collectionPathOf(const QString &path);
};
