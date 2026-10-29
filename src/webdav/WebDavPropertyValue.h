#pragma once

#include <QString>
#include <QList>
#include <QHash>

struct WebDavChildElement
{
    QString namespaceUri;
    QString localName;
    QHash<QString, QString> attributes;
};

struct WebDavPropertyValue
{
    QString text;
    QList<WebDavChildElement> childElements;
};
