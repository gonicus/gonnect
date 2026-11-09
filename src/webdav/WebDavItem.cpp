#include "WebDavItem.h"

#include <QUrl>
#include <QTimeZone>
#include <QLoggingCategory>

#include <array>
#include <algorithm>

Q_LOGGING_CATEGORY(lcWebDavItem, "gonnect.app.webdav.WebDavItem")

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

QString WebDavItem::displayName() const
{
    const auto text = property("DAV:", "displayname").text.trimmed();
    return text.isEmpty() ? normalizedPath().section('/', -1) : text;
}

QDateTime WebDavItem::lastModified() const
{
    const auto text = property("DAV:", "getlastmodified").text.trimmed();
    if (text.isEmpty()) {
        // getlastmodified is optional, thus may be empty
        return QDateTime();
    }

    auto dateTime = parseHttpDate(text);
    if (!dateTime.isValid()) {
        // Fallback for date string with numeric zone instead of "GMT"
        dateTime = QDateTime::fromString(text, Qt::RFC2822Date);
    }

    if (!dateTime.isValid()) {
        qCDebug(lcWebDavItem) << "Unable to parse date string" << text;
    }

    // Some servers report epoch for unknown modification time
    return dateTime.isValid() && dateTime.toSecsSinceEpoch() == 0 ? QDateTime() : dateTime;
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

int WebDavItem::parseDigits(QStringView digits)
{
    int value = 0;
    for (const QChar c : digits) {
        if (c < u'0' || c > u'9') {
            return -1;
        }
        value = value * 10 + (c.unicode() - u'0');
    }
    return value;
}

QDateTime WebDavItem::parseHttpDate(QStringView text)
{
    // Sample string: "Sun, 06 Nov 1994 08:49:37 GMT" (RFC 4918 section 15.7)

    static constexpr std::array<QStringView, 12> monthNames = {
        u"Jan", u"Feb", u"Mar", u"Apr", u"May", u"Jun",
        u"Jul", u"Aug", u"Sep", u"Oct", u"Nov", u"Dec",
    };

    if (text.size() != 29 || text.at(3) != u',' || text.sliced(25) != QLatin1StringView(" GMT")) {
        return QDateTime();
    }

    const auto monthIt = std::ranges::find(monthNames, text.sliced(8, 3));
    if (monthIt == monthNames.end()) {
        return QDateTime();
    }

    const QDate date(parseDigits(text.sliced(12, 4)),
                     static_cast<int>(std::distance(monthNames.begin(), monthIt)) + 1,
                     parseDigits(text.sliced(5, 2)));
    const QTime time(parseDigits(text.sliced(17, 2)), parseDigits(text.sliced(20, 2)),
                     parseDigits(text.sliced(23, 2)));

    if (!date.isValid() || !time.isValid()) {
        return QDateTime();
    }

    return QDateTime(date, time, QTimeZone::UTC);
}
