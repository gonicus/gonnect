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
        if (child.first == "DAV:" && child.second == "collection") {
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
