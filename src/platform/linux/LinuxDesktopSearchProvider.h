#pragma once

#include <QObject>
#include <QHash>
#include <QDBusMetaType>
#include "SearchProvider.h"
#include "Contact.h"

class ShellSearchProvider2Adapter;
class KRunner2Adapter;

struct Action
{
    QString id;
    QString iconSource;
    QString text;
};

typedef QList<Action> ActionList;

struct SearchResult
{
    QString contactId;
    QString name;
    QString company;
    QString phoneNumber;
    Contact::NumberType type;
    bool sipSubscriptable = false;
};

struct RemoteMatch
{
    QString id;
    QString text;
    QString iconName;
    int categoryRelevance = 0;
    qreal relevance = 0;
    QVariantMap properties;
};

typedef QList<RemoteMatch> RemoteMatches;

inline QDBusArgument &operator<<(QDBusArgument &argument, const RemoteMatch &match)
{
    argument.beginStructure();
    argument << match.id;
    argument << match.text;
    argument << match.iconName;
    argument << match.categoryRelevance;
    argument << match.relevance;
    argument << match.properties;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, RemoteMatch &match)
{
    argument.beginStructure();
    argument >> match.id;
    argument >> match.text;
    argument >> match.iconName;
    argument >> match.categoryRelevance;
    argument >> match.relevance;
    argument >> match.properties;
    argument.endStructure();

    return argument;
}

inline QDBusArgument &operator<<(QDBusArgument &argument, const Action &action)
{
    argument.beginStructure();
    argument << action.id;
    argument << action.iconSource;
    argument << action.text;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, Action &action)
{
    argument.beginStructure();
    argument >> action.id;
    argument >> action.iconSource;
    argument >> action.text;
    argument.endStructure();

    return argument;
}

class LinuxDesktopSearchProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit LinuxDesktopSearchProvider(QObject *parent = nullptr);
    ~LinuxDesktopSearchProvider();

public slots:
    // GNOME search provider
    void ActivateResult(const QString &identifier, const QStringList &terms, uint timestamp);
    QStringList GetInitialResultSet(const QStringList &terms);
    QList<QVariantMap> GetResultMetas(const QStringList &identifiers);
    QStringList GetSubsearchResultSet(const QStringList &previous_results,
                                      const QStringList &terms);
    void LaunchSearch(const QStringList &terms, uint timestamp);

    // KDE Krunner
    void Teardown();
    QVariantMap Config();
    ActionList Actions();
    void Run(const QString &matchId, const QString &actionId);
    RemoteMatches Match(const QString &query);

signals:
    void activateSearch(QString query);

private:
    ShellSearchProvider2Adapter *m_searchAdapter = nullptr;
    KRunner2Adapter *m_krunnerAdapter = nullptr;

    QHash<QString, SearchResult *> m_searchResults;

    bool m_isRegistered = false;
};

Q_DECLARE_METATYPE(Action)
Q_DECLARE_METATYPE(ActionList)
Q_DECLARE_METATYPE(RemoteMatch)
Q_DECLARE_METATYPE(RemoteMatches)
