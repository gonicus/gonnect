#include "WebDavPropfindParser.h"

#include <QXmlStreamReader>

using namespace Qt::StringLiterals;

WebDavPropfindParser::WebDavPropfindParser() { }

QByteArray WebDavPropfindParser::buildPropfindBody(const QList<WebDavPropertyRequest> &properties)
{
    QHash<QString, QString> namespaces;
    for (const auto &p : std::as_const(properties)) {
        namespaces.insert(p.prefix, p.namespaceUri);
    }

    QByteArray body = R"(<?xml version="1.0" encoding="utf-8"?>
        <D:propfind xmlns:D="DAV:")"_ba;

    // Add namespaces
    QHashIterator it(namespaces);
    while (it.hasNext()) {
        it.next();

        if (it.key() == "D") {
            continue; // already declarated
        }
        body += " xmlns:" + it.key().toUtf8() + "=\"" + it.value().toUtf8() + R"(")"_ba;
    }

    body += R"(>
        <D:prop>
    )"_ba;

    // Add properties
    for (const auto &p : std::as_const(properties)) {
        body += "<" + p.prefix.toUtf8() + ":" + p.name.toUtf8() + " />\n";
    }

    body += R"(</D:prop>
        </D:propfind>)"_ba;
    return body;
}

QByteArray WebDavPropfindParser::buildMultigetBody(const QString &rootPrefix,
                                                   const QString &rootNamespaceUri,
                                                   const QString &rootElementName,
                                                   const QList<WebDavPropertyRequest> &properties,
                                                   const QStringList &hrefs)
{
    // Collect namespaces
    QHash<QString, QString> namespaces;
    namespaces.insert(rootPrefix, rootNamespaceUri);

    for (const auto &p : properties) {
        namespaces.insert(p.prefix, p.namespaceUri);
    }

    // Build body
    QByteArray body = R"(<?xml version="1.0" encoding="utf-8"?>)"_ba;
    body += (QString("<%1:%2 xmlns:D=\"DAV:\"").arg(rootPrefix, rootElementName).toUtf8());

    // Add namespaces
    QHashIterator it(namespaces);
    while (it.hasNext()) {
        it.next();
        if (it.key() == "D") {
            continue;
        }
        body += QString(" xmlns:%1=\"%2\"").arg(it.key(), it.value()).toUtf8();
    }

    // Add props
    body += R"(><D:prop>)"_ba;
    for (const auto &p : properties) {
        body += QString("<%1:%2 />").arg(p.prefix, p.name).toUtf8();
    }
    body += R"(</D:prop>)"_ba;

    // Add hrefs
    for (const auto &href : hrefs) {
        body += QString("<D:href>%1</D:href>").arg(href.toHtmlEscaped()).toUtf8();
    }

    body += QString("</%1:%2>").arg(rootPrefix, rootElementName).toUtf8();

    return body;
}

QList<WebDavItem> WebDavPropfindParser::parseMultiStatus(const QByteArray &xmlData)
{

    QList<WebDavItem> items;
    QXmlStreamReader reader(xmlData);

    // First element is <multistatus>
    if (!reader.readNextStartElement()) {
        return items;
    }

    // Every child is <response>
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() == QLatin1String("DAV:")
            && reader.name() == QLatin1String("response")) {
            items.append(parseResponse(reader));
        } else {
            // Unknown element
            reader.skipCurrentElement();
        }
    }

    return items;
}

WebDavPropertyValue WebDavPropfindParser::parsePropertyValue(QXmlStreamReader &reader) const
{
    WebDavPropertyValue value;

    while (reader.readNext() != QXmlStreamReader::Invalid) {
        if (reader.isEndElement()) {
            break;
        }

        if (reader.isStartElement()) {
            value.childElements.append(
                    { reader.namespaceUri().toString(), reader.name().toString() });
            reader.skipCurrentElement();
        } else if (reader.isCharacters() && !reader.isWhitespace()) {
            value.text += reader.text().toString();
        }
    }

    return value;
}

void WebDavPropfindParser::parseProp(QXmlStreamReader &reader,
                                     QHash<QString, WebDavPropertyValue> &result) const
{
    // reader on star tag of <prop> - every child is single property
    while (reader.readNextStartElement()) {
        const QString key =
                QString("%1|%2").arg(reader.namespaceUri().toString(), reader.name().toString());
        result.insert(key, parsePropertyValue(reader));
    }
}

int WebDavPropfindParser::parseHttpStatusLine(const QString &statusLine) const
{
    // Format e.g. "HTTP/1.1 200 OK" - only care about status code
    const auto parts = statusLine.split(' ');
    return parts.size() > 1 ? parts.at(1).toInt() : 0;
}

void WebDavPropfindParser::parsePropstat(QXmlStreamReader &reader, WebDavItem &item) const
{
    QHash<QString, WebDavPropertyValue> pendingProperties;
    int statusCode = 0;

    while (reader.readNextStartElement()) {
        const bool isDavNamespace = reader.namespaceUri() == QLatin1String("DAV:");

        if (isDavNamespace && reader.name() == QLatin1String("prop")) {
            parseProp(reader, pendingProperties);
        } else if (isDavNamespace && reader.name() == QLatin1String("status")) {
            statusCode = parseHttpStatusLine(reader.readElementText());
        } else {
            reader.skipCurrentElement();
        }
    }

    // Only use propstat block if successful (2xx)
    if (statusCode >= 200 && statusCode < 300) {
        QHashIterator it(pendingProperties);
        while (it.hasNext()) {
            it.next();
            item.properties.insert(it.key(), it.value());
        }

        if (item.httpStatus == -1) {
            item.httpStatus = statusCode;
        }
    }
}

WebDavItem WebDavPropfindParser::parseResponse(QXmlStreamReader &reader) const
{
    WebDavItem item;

    // reader on start tag of <response>
    while (reader.readNextStartElement()) {
        const bool isDavNamespace = reader.namespaceUri() == QLatin1String("DAV:");
        const auto name = reader.name();

        if (isDavNamespace && name == QLatin1String("href")) {
            item.href = reader.readElementText();

        } else if (isDavNamespace && name == QLatin1String("status")) {
            item.httpStatus = parseHttpStatusLine(reader.readElementText());

        } else if (isDavNamespace && name == QLatin1String("propstat")) {
            parsePropstat(reader, item);

        } else {
            reader.skipCurrentElement();
        }
    }

    return item;
}
