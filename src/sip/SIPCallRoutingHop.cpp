#include "SIPCallRoutingHop.h"

#include <QObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QStringList>

Q_LOGGING_CATEGORY(lcSIPCallRoutingHop, "gonnect.sip.routinghop")

QString SIPCallRoutingHop::hopReasonToString(const QString &reason)
{
    const QString r = reason.toLower();

    if (r == "unconditional") {
        return QObject::tr("redirected");
    } else if (r == "no-answer") {
        return QObject::tr("not answered");
    } else if (r == "user-busy" || r == "mobile-busy") {
        return QObject::tr("busy");
    } else if (r == "away") {
        return QObject::tr("away");
    } else if (r == "follow-me") {
        return QObject::tr("forwarded");
    } else if (r == "time-of-day") {
        return QObject::tr("time base redirection");
    } else if (r == "do-not-disturb") {
        return QObject::tr("do not disturb");
    } else if (r == "deflection" || r == "deflection-immediate") {
        return QObject::tr("rejected");
    }

    return "";
}

QString SIPCallRoutingHop::hopCauseToString(int cause)
{
    switch (cause) {
    case 302:
        return QObject::tr("redirected");
    case 408:
        return QObject::tr("not answered");
    case 486:
        return QObject::tr("busy");
    case 487:
        return QObject::tr("aborted");
    case 480:
        return QObject::tr("not reachable");
    default:
        return cause > 0 ? QObject::tr("Code %1").arg(cause) : "";
    }
}

QStringList SIPCallRoutingHop::splitHeaderEntries(const QString &value)
{
    static const QRegularExpression entryRe(R"(<[^>]*>[^,]*)");
    QStringList entries;

    auto it = entryRe.globalMatch(value);
    while (it.hasNext()) {
        entries.append(it.next().captured(0).trimmed());
    }

    return entries;
}

SIPCallRoutingHop SIPCallRoutingHop::parseHistoryInfoEntry(const QString &value)
{
    static const QRegularExpression uriRe(R"(<([^>]+)>)");
    static const QRegularExpression causeRe(R"(cause=(\d+))",
                                            QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression indexRe(R"(index=([\d.]+))",
                                            QRegularExpression::CaseInsensitiveOption);

    SIPCallRoutingHop hop;
    hop.diversion = false;

    const auto uriMatch = uriRe.match(value);
    if (!uriMatch.hasMatch()) {
        qCWarning(lcSIPCallRoutingHop)
                << "History-Info header entry has no URI - skipping:" << value;
        return hop;
    }
    hop.uri = uriMatch.captured(1);

    const auto causeMatch = causeRe.match(value);
    if (causeMatch.hasMatch()) {
        hop.reason = causeMatch.captured(1).toInt();
        hop.reasonText = hopCauseToString(hop.reason);
    }

    const auto indexMatch = indexRe.match(value);
    if (indexMatch.hasMatch()) {
        hop.index = indexMatch.captured(1);
    }

    qCDebug(lcSIPCallRoutingHop) << "Adding call routing hop from History-Info header:" << value;
    return hop;
}

SIPCallRoutingHop SIPCallRoutingHop::parseDiversionEntry(const QString &value)
{
    static const QRegularExpression uriRe(R"(<([^>]+)>)");
    static const QRegularExpression reasonRe(R"(reason=\"?([^";,\r\n]+)\"?)",
                                             QRegularExpression::CaseInsensitiveOption);

    SIPCallRoutingHop hop;
    hop.diversion = true;

    const auto uriMatch = uriRe.match(value);
    if (!uriMatch.hasMatch()) {
        qCWarning(lcSIPCallRoutingHop) << "Diversion header entry has no URI - skipping:" << value;
        return hop;
    }
    hop.uri = uriMatch.captured(1);

    const auto reasonMatch = reasonRe.match(value);
    if (reasonMatch.hasMatch()) {
        hop.reasonText = hopReasonToString(reasonMatch.captured(1).trimmed());
    }

    qCDebug(lcSIPCallRoutingHop) << "Adding call routing hop from Diversion header:" << value;
    return hop;
}

QList<SIPCallRoutingHop> SIPCallRoutingHop::parse(const QStringList &historyInfoHeaders,
                                                  const QStringList &diversionHeaders)
{
    QList<SIPCallRoutingHop> historyInfoHops;
    for (const QString &header : historyInfoHeaders) {
        const auto she = splitHeaderEntries(header);
        for (const QString &entry : std::as_const(she)) {
            const QString value = entry.trimmed();
            if (value.isEmpty()) {
                continue;
            }
            const auto hop = parseHistoryInfoEntry(value);
            if (!hop.uri.isEmpty()) {
                historyInfoHops.append(hop);
            }
        }
    }

    // Prefer history-info as defined by qualified RFC
    if (!historyInfoHops.isEmpty()) {
        return historyInfoHops;
    }

    QList<SIPCallRoutingHop> diversionHops;
    for (const QString &header : diversionHeaders) {
        const auto she = splitHeaderEntries(header);
        for (const QString &entry : std::as_const(she)) {
            const QString value = entry.trimmed();
            if (value.isEmpty()) {
                continue;
            }
            const auto hop = parseDiversionEntry(value);
            if (!hop.uri.isEmpty()) {
                diversionHops.append(hop);
            }
        }
    }

    // Diversion headers are ordered most-recent-first; return them chronologically.
    std::reverse(diversionHops.begin(), diversionHops.end());
    return diversionHops;
}
