#include <QLoggingCategory>
#include <QRegularExpression>
#include <QMetaEnum>

#include "SIPCallInfo.h"

Q_LOGGING_CATEGORY(lcSIPCallInfo, "gonnect.sip.callinfo")

QStringList SIPCallInfo::splitHeaderEntries(const QString &value)
{
    static const QRegularExpression entryRe(R"(<[^>]*>[^,]*)");
    QStringList entries;

    auto it = entryRe.globalMatch(value);
    while (it.hasNext()) {
        entries.append(it.next().captured(0).trimmed());
    }

    return entries;
}

SIPCallInfo::Security SIPCallInfo::parseSecurity(const QString &value)
{
    if (value.compare("NotAuthenticated", Qt::CaseInsensitive) == 0) {
        return Security::NotAuthenticated;
    }
    if (value.compare("Authenticated", Qt::CaseInsensitive) == 0) {
        return Security::Authenticated;
    }
    if (value.compare("Encrypted", Qt::CaseInsensitive) == 0) {
        return Security::Encrypted;
    }

    qCWarning(lcSIPCallInfo) << "ignoring unknown Call-Info security value:" << value;

    return Security::Unknown;
}

QString SIPCallInfo::securityToString(Security security)
{
    return QMetaEnum::fromType<Security>().valueToKey(security);
}

SIPCallInfo::UiState SIPCallInfo::parseUiState(const QString &value)
{
    if (value.compare("ringout", Qt::CaseInsensitive) == 0) {
        return UiState::Ringout;
    }
    if (value.compare("busy", Qt::CaseInsensitive) == 0) {
        return UiState::Busy;
    }

    qCWarning(lcSIPCallInfo) << "ignoring unknown Call-Info ui-state value:" << value;

    return UiState::None;
}

QString SIPCallInfo::uiStateToString(UiState uiState)
{
    return QMetaEnum::fromType<UiState>().valueToKey(uiState);
}

SIPCallInfo::Priority SIPCallInfo::parsePriority(const QString &value)
{
    if (value.compare("urgent", Qt::CaseInsensitive) == 0) {
        return Priority::Urgent;
    }
    if (value.compare("emergency", Qt::CaseInsensitive) == 0) {
        return Priority::Emergency;
    }
    if (value.compare("normal", Qt::CaseInsensitive) == 0) {
        return Priority::Normal;
    }

    qCWarning(lcSIPCallInfo) << "ignoring unknown Call-Info priority value:" << value;

    return Priority::Normal;
}

QString SIPCallInfo::priorityToString(Priority priority)
{
    return QMetaEnum::fromType<Priority>().valueToKey(priority);
}

SIPCallInfo SIPCallInfo::parse(const QStringList &callInfoHeaders)
{
    static const QRegularExpression uriRe(R"(<\s*urn:x-cisco-remotecc:callinfo\s*>)",
                                          QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression paramRe(R"(([\w.!%*+`'~-]+)\s*=\s*([^;]*))");

    SIPCallInfo info;

    for (const QString &header : callInfoHeaders) {
        const auto entries = splitHeaderEntries(header);

        for (const QString &entry : entries) {
            if (!uriRe.match(entry).hasMatch()) {
                continue;
            }

            info.m_valid = true;

            auto it = paramRe.globalMatch(entry);
            while (it.hasNext()) {
                const auto match = it.next();
                const QString name = match.captured(1).trimmed();
                const QString value = match.captured(2).trimmed();

                if (name.compare("security", Qt::CaseInsensitive) == 0) {
                    info.m_security = parseSecurity(value);
                } else if (name.compare("ui-state", Qt::CaseInsensitive) == 0) {
                    info.m_uiState = parseUiState(value);
                } else if (name.compare("priority", Qt::CaseInsensitive) == 0) {
                    info.m_priority = parsePriority(value);
                } else if (name.compare("gci", Qt::CaseInsensitive) == 0) {
                    info.m_gci = value;
                }

                // TODO: there are more (orientation, call-instance, etc.)
            }
        }
    }

    return info;
}
