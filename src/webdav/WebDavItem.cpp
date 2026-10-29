#include "WebDavItem.h"

#include <QUrl>

WebDavPropertyValue WebDavItem::property(const QString &namespaceUri, const QString &name) const
{
    return properties.value(QString("%1|%2").arg(namespaceUri, name));
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
    return QDateTime::fromString(property("DAV:", "getlastmodified").text, Qt::RFC2822Date);
}
