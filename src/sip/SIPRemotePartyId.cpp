#include <QLoggingCategory>
#include <QRegularExpression>
#include <QMetaEnum>

#include "SIPRemotePartyId.h"

Q_LOGGING_CATEGORY(lcSIPRemotePartyId, "gonnect.sip.rpid")

QStringList SIPRemotePartyId::splitHeaderEntries(const QString &value)
{
    QStringList entries;
    QString current;
    bool inQuotes = false;
    bool inUri = false;

    for (const QChar c : value) {
        if (c == '"' && !inUri) {
            inQuotes = !inQuotes;
        } else if (c == '<' && !inQuotes) {
            inUri = true;
        } else if (c == '>' && !inQuotes) {
            inUri = false;
        } else if (c == ',' && !inQuotes && !inUri) {
            if (!current.trimmed().isEmpty()) {
                entries.append(current.trimmed());
            }
            current.clear();
            continue;
        }
        current.append(c);
    }

    if (!current.trimmed().isEmpty()) {
        entries.append(current.trimmed());
    }

    return entries;
}

SIPRemotePartyId::Privacy SIPRemotePartyId::parsePrivacy(const QString &value)
{
    if (value.compare("off", Qt::CaseInsensitive) == 0) {
        return Privacy::Off;
    }
    if (value.compare("name", Qt::CaseInsensitive) == 0) {
        return Privacy::Name;
    }
    if (value.compare("uri", Qt::CaseInsensitive) == 0) {
        return Privacy::Uri;
    }
    if (value.compare("full", Qt::CaseInsensitive) == 0) {
        return Privacy::Full;
    }

    qCWarning(lcSIPRemotePartyId) << "unknown Remote-Party-ID privacy value:" << value
                                  << "-> assuming Privacy::Full";
    return Privacy::Full;
}

SIPRemotePartyId::Party SIPRemotePartyId::parseParty(const QString &value)
{
    if (value.compare("calling", Qt::CaseInsensitive) == 0) {
        return Party::Calling;
    }
    if (value.compare("called", Qt::CaseInsensitive) == 0) {
        return Party::Called;
    }

    return Party::Unknown;
}

QString SIPRemotePartyId::privacyToString(Privacy privacy)
{
    return QMetaEnum::fromType<Privacy>().valueToKey(privacy);
}

SIPRemotePartyId SIPRemotePartyId::parseEntry(const QString &value)
{
    // "Bob Jones" <sip:9728135001@host;x-cisco-callback-number=972…>;party=called;privacy=off
    static const QRegularExpression entryRe(
            R"RE(^\s*(?:"([^"]*)"|([^<;]*?))?\s*<([^>]*)>\s*(.*)$)RE");
    // Bare addr-spec form without angle brackets: sip:user@host;param=value
    static const QRegularExpression bareRe(R"RE(^\s*([^;]+)\s*(.*)$)RE");

    SIPRemotePartyId rpid;

    QString uriPart;
    QString paramPart;

    const auto match = entryRe.match(value);
    if (match.hasMatch()) {
        rpid.m_rawDisplayName =
                match.captured(1).isNull() ? match.captured(2).trimmed() : match.captured(1);
        uriPart = match.captured(3).trimmed();
        paramPart = match.captured(4);
    } else {
        const auto bare = bareRe.match(value);
        if (!bare.hasMatch()) {
            qCWarning(lcSIPRemotePartyId) << "unparsable Remote-Party-ID entry:" << value;
            return rpid;
        }
        uriPart = bare.captured(1).trimmed();
        paramPart = bare.captured(2);
    }

    if (uriPart.isEmpty()) {
        qCWarning(lcSIPRemotePartyId) << "Remote-Party-ID entry without URI:" << value;
        return rpid;
    }

    // Split the URI from its own parameters - x-cisco-callback-number lives inside the brackets
    const auto uriTokens = uriPart.split(';');
    rpid.m_uri = uriTokens.first().trimmed();

    for (qsizetype i = 1; i < uriTokens.size(); ++i) {
        const QString token = uriTokens.at(i).trimmed();
        const qsizetype eq = token.indexOf('=');
        if (eq < 0) {
            continue;
        }

        const QString name = token.left(eq).trimmed();
        const QString val = token.mid(eq + 1).trimmed();
        if (name.compare("x-cisco-callback-number", Qt::CaseInsensitive) == 0) {
            rpid.m_callbackNumber = val;
        }
    }

    // The user part of the URI carries the number
    const qsizetype colon = rpid.m_uri.indexOf(':');
    const QString afterScheme = colon >= 0 ? rpid.m_uri.mid(colon + 1) : rpid.m_uri;
    const qsizetype at = afterScheme.indexOf('@');
    rpid.m_rawNumber = at >= 0 ? afterScheme.left(at) : afterScheme;

    // Header parameters
    const auto params = paramPart.split(';');
    for (const QString &raw : std::as_const(params)) {
        const QString token = raw.trimmed();
        if (token.isEmpty()) {
            continue;
        }

        const qsizetype eq = token.indexOf('=');
        if (eq < 0) {
            continue;
        }

        const QString name = token.left(eq).trimmed();
        const QString val = token.mid(eq + 1).trimmed();

        if (name.compare("privacy", Qt::CaseInsensitive) == 0) {
            rpid.m_privacy = parsePrivacy(val);
        } else if (name.compare("party", Qt::CaseInsensitive) == 0) {
            rpid.m_party = parseParty(val);
        } else if (name.compare("screen", Qt::CaseInsensitive) == 0) {
            rpid.m_screened = val.compare("yes", Qt::CaseInsensitive) == 0;
        }
    }

    rpid.m_valid = true;

    return rpid;
}

QList<SIPRemotePartyId> SIPRemotePartyId::parse(const QStringList &headers)
{
    QList<SIPRemotePartyId> result;

    for (const QString &header : std::as_const(headers)) {
        const auto entries = splitHeaderEntries(header);
        for (const QString &entry : std::as_const(entries)) {
            const auto rpid = parseEntry(entry);
            if (rpid.m_valid) {
                result.append(rpid);
            }
        }
    }

    return result;
}
