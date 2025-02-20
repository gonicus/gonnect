#include <QtDBus/QDBusConnection>

#include "SIPAccountManager.h"
#include "SIPManager.h"
#include "SIPCallManager.h"
#include "ShellSearchProvider2Adapter.h"
#include "SearchProvider.h"
#include "AddressBook.h"
#include "KRunnerAdapter.h"

Q_LOGGING_CATEGORY(lcSearchProvider, "gonnect.dbus.searchprovider")

SearchProvider::SearchProvider(QObject *parent) : QObject(parent)
{
    m_searchAdapter = new ShellSearchProvider2Adapter(this);
    m_krunnerAdapter = new KRunner2Adapter(this);

    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        m_isRegistered = con.registerObject("/de/gonicus/gonnect/SearchProvider", this)
                && con.registerService("de.gonicus.gonnect");
    }
}

SearchProvider::~SearchProvider()
{
    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        con.unregisterObject("/de/gonicus/gonnect/SearchProvider", QDBusConnection::UnregisterTree);
        con.unregisterService("de.gonicus.gonnect");
    }

    qDeleteAll(m_searchResults);
    m_searchResults.clear();
}

void SearchProvider::ActivateResult(const QString &identifier, const QStringList &terms,
                                    uint timestamp)
{
    Q_UNUSED(terms)
    Q_UNUSED(timestamp)
    qCDebug(lcSearchProvider) << "got dial request for" << identifier;

    if (auto sr = m_searchResults.value(identifier, nullptr)) {
        const auto &accounts = SIPAccountManager::instance().accounts();
        if (!accounts.isEmpty()) {
            SIPCallManager::instance().call(accounts.first()->id(), sr->phoneNumber, sr->contactId);
        }
    }

    qDeleteAll(m_searchResults);
    m_searchResults.clear();
}

QStringList SearchProvider::GetInitialResultSet(const QStringList &terms)
{
    qDeleteAll(m_searchResults);
    m_searchResults.clear();
    return GetSubsearchResultSet({}, terms);
}

QStringList SearchProvider::GetSubsearchResultSet(const QStringList &previous_results,
                                                  const QStringList &terms)
{
    Q_UNUSED(previous_results)
    qCDebug(lcSearchProvider) << "got search request for:" << terms.join(" ");

    QStringList resultIds;

    auto results = AddressBook::instance().search(terms.join(" "));
    for (auto contact : std::as_const(results)) {

        auto const uris = contact->phoneNumbers();
        for (auto &uri : std::as_const(uris)) {
            QString id = contact->company() + "#" + uri.number;
            if (uri.isSipSubscriptable) {
                id += QString("#%1").arg(SIPManager::instance().buddyStatus(uri.number));
            }

            if (!m_searchResults.contains(id)) {
                auto sr = new SearchResult();
                sr->contactId = contact->id();
                sr->name = contact->name();
                sr->company = contact->company();
                sr->type = uri.type;
                sr->phoneNumber = uri.number;
                sr->sipSubscriptable = uri.isSipSubscriptable;
                m_searchResults.insert(id, sr);
            }

            resultIds.push_back(id);
        }
    }

    qCDebug(lcSearchProvider) << "found" << resultIds.length() << "contacts";

    return resultIds;
}

QList<QVariantMap> SearchProvider::GetResultMetas(const QStringList &identifiers)
{
    QList<QVariantMap> results;
    qCDebug(lcSearchProvider) << "got meta result request for" << identifiers;

    for (auto &id : std::as_const(identifiers)) {
        if (auto sr = m_searchResults.value(id, nullptr)) {
            QString displayName = sr->name;
            if (!sr->company.isEmpty()) {
                displayName += " - " + sr->company;
            }

            QString dsc = sr->phoneNumber;
            if (sr->sipSubscriptable) {
                switch (SIPManager::instance().buddyStatus(sr->phoneNumber)) {
                case SIPBuddyState::UNKNOWN:
                    break;
                case SIPBuddyState::UNAVAILABLE:
                    dsc = "⚫ " + dsc;
                    break;
                case SIPBuddyState::BUSY:
                    dsc = "🔴 " + dsc;
                    break;
                default:
                    dsc = "🟢 " + dsc;
                    break;
                }
            }

            results.push_back(QVariantMap({
                    { "id", id },
                    { "name", displayName },
                    { "icon", "call-start-symbolic" },
                    { "description", dsc },
            }));

        } else {
            qCCritical(lcSearchProvider) << "identifier not found - ignoring result request";
        }
    }

    return results;
}

void SearchProvider::LaunchSearch(const QStringList &terms, uint timestamp)
{
    Q_UNUSED(timestamp)
    qCDebug(lcSearchProvider) << "sending launch search request to search dialog:"
                              << terms.join(" ");
    emit activateSearch(terms.join(" "));
    qDeleteAll(m_searchResults);
    m_searchResults.clear();
}

void SearchProvider::Teardown()
{
    qDeleteAll(m_searchResults);
    m_searchResults.clear();
}

QVariantMap SearchProvider::Config()
{
    return QVariantMap();
}

ActionList SearchProvider::Actions()
{
    return ActionList({ { "call", tr("Call"), "call-start-symbolic" } });
}

void SearchProvider::Run(const QString &matchId, const QString &actionId)
{
    if (actionId == "call") {
        ActivateResult(matchId, {}, 0);
    }
}

RemoteMatches SearchProvider::Match(const QString &query)
{
    auto resultIds = GetSubsearchResultSet({}, { query });

    RemoteMatches matches;

    for (auto &id : std::as_const(resultIds)) {
        if (auto sr = m_searchResults.value(id, nullptr)) {
            QString displayName = sr->name;
            if (!sr->company.isEmpty()) {
                displayName += " - " + sr->company;
            }

            QString dsc = sr->phoneNumber;
            if (sr->sipSubscriptable) {
                switch (SIPManager::instance().buddyStatus(sr->phoneNumber)) {
                case SIPBuddyState::UNKNOWN:
                    break;
                case SIPBuddyState::UNAVAILABLE:
                    dsc = "⚫ " + dsc;
                    break;
                case SIPBuddyState::BUSY:
                    dsc = "🔴 " + dsc;
                    break;
                default:
                    dsc = "🟢 " + dsc;
                    break;
                }
            }

            RemoteMatch match;
            match.id = id;
            match.iconName = "call-start-symbolic";
            match.text = displayName;
            match.properties = { { "subtext", dsc },
                                 { "actions", QStringList({ "call" }) },
                                 { "multiline", false } };

            matches.push_back(match);
        } else {
            qCCritical(lcSearchProvider) << "identifier not found - ignoring result request";
        }
    }
    return matches;
}
