#include "WebDavItem.h"

#include <QUrl>
#include <QLocale>
#include <QTimeZone>

WebDavPropertyValue WebDavItem::property(const QString &namespaceUri, const QString &name) const
{
    return properties.value(QString("%1|%2").arg(namespaceUri, name));
}

bool WebDavItem::hasProperty(const QString &namespaceUri, const QString &name) const
{
    return properties.contains(QString("%1|%2").arg(namespaceUri, name));
}

bool WebDavItem::isCollection() const
{
    const auto resourceType = property("DAV:", "resourcetype");
    for (const auto &child : std::as_const(resourceType.childElements)) {
        if (child.namespaceUri == "DAV:" && child.localName == "collection") {
            return true;
        }
    }
    return false;
}

bool WebDavItem::isCalendarCollection() const
{
    const auto resourceType = property("DAV:", "resourcetype");
    for (const auto &child : std::as_const(resourceType.childElements)) {
        if (child.namespaceUri == "urn:ietf:params:xml:ns:caldav"
            && child.localName == "calendar") {
            return true;
        }
    }
    return false;
}

bool WebDavItem::isContactCollection() const
{
    const auto resourceType = property("DAV:", "resourcetype");
    for (const auto &child : std::as_const(resourceType.childElements)) {
        if (child.namespaceUri == "urn:ietf:params:xml:ns:carddav"
            && child.localName == "addressbook") {
            return true;
        }
    }
    return false;
}

bool WebDavItem::supportsComponent(const QString &componentName) const
{
    const auto componentSet =
            property("urn:ietf:params:xml:ns:caldav", "supported-calendar-component-set");
    for (const auto &child : std::as_const(componentSet.childElements)) {
        if (child.namespaceUri == "urn:ietf:params:xml:ns:caldav" && child.localName == "comp"
            && child.attributes.value("name") == componentName) {
            return true;
        }
    }
    return false;
}

QString WebDavItem::name() const
{
    return QUrl(href).fileName();
}

QDateTime WebDavItem::lastModified() const
{
    // RFC 1123 date string (according to RFC 4918 sec. 15.7)
    const auto text = property("DAV:", "getlastmodified").text.trimmed();
    auto dateTime = QLocale::c().toDateTime(text, "ddd, dd MMM yyyy HH:mm:ss 'GMT'");
    if (dateTime.isValid()) {
        dateTime.setTimeZone(QTimeZone::UTC);
        return dateTime;
    }

    // Fallback for date string with numeric zone instead of "GMT"
    return QDateTime::fromString(text, Qt::RFC2822Date);
}

QString WebDavItem::syncToken() const
{
    return property("DAV:", "sync-token").text.trimmed();
}

QString WebDavItem::normalizedPath() const
{
    return normalizedPath(QUrl(href));
}

QString WebDavItem::normalizedPath(const QUrl &url)
{
    return url.adjusted(QUrl::NormalizePathSegments | QUrl::StripTrailingSlash)
            .path(QUrl::FullyDecoded);
}

QString WebDavItem::collectionPathOf(const QString &path)
{
    return path.left(path.lastIndexOf('/'));
}
