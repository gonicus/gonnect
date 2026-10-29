#pragma once

#include "WebDavItem.h"
#include "WebDavPropertyRequest.h"

#include <QByteArray>
#include <QList>

class QXmlStreamReader;

class WebDavPropfindParser
{

public:
    WebDavPropfindParser();

    QByteArray buildPropfindBody(const QList<WebDavPropertyRequest> &properties);
    QByteArray buildMultigetBody(const QString &rootPrefix, const QString &rootNamespaceUri,
                                 const QString &rootElementName,
                                 const QList<WebDavPropertyRequest> &properties,
                                 const QStringList &hrefs);
    QList<WebDavItem> parseMultiStatus(const QByteArray &xmlData);

private:
    WebDavPropertyValue parsePropertyValue(QXmlStreamReader &reader) const;
    void parseProp(QXmlStreamReader &reader, QHash<QString, WebDavPropertyValue> &result) const;
    int parseHttpStatusLine(const QString &statusLine) const;
    void parsePropstat(QXmlStreamReader &reader, WebDavItem &item) const;
    WebDavItem parseResponse(QXmlStreamReader &reader) const;
};
