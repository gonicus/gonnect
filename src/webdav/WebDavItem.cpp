#include "WebDavItem.h"

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
