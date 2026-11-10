#pragma once

#include "CardDAVAddressBookFeederConfig.h"
#include "IAddressBookFeeder.h"
#include "Contact.h"
#include "BlockInfo.h"
#include "WebDavItem.h"
#include "WebDavSyncResult.h"

#include <QObject>
#include <QSet>
#include <QElapsedTimer>
#include <QCoroCore>

#include <expected>

class AddressBook;
class AddressBookManager;
class WebDavClient;

class CardDAVAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit CardDAVAddressBookFeeder(const QString &group, const int retryCount,
                                      const int retryInterval,
                                      AddressBookManager *parent = nullptr);

    ~CardDAVAddressBookFeeder();

    void process() override;
    QUrl networkCheckURL() const override;

private:
    void init();

    QCoro::Task<> run();
    [[nodiscard]] QCoro::Task<WebDavSyncResult>
    syncContactsCollection(const QString &collectionPath, int depth);
    /// If hrefsAreKnownChanged is true every card is parsed again (skipped cache heuristics)
    [[nodiscard]] QCoro::Task<WebDavSyncResult> fetchContacts(const QString &collectionPath,
                                                              const QStringList &hrefs,
                                                              bool hrefsAreKnownChanged = false);

    /// Sync address book found in parent.
    [[nodiscard]] QCoro::Task<WebDavSyncResult> syncChildAddressBook(const WebDavItem &item,
                                                                     int depth);

    /// Full sync - unchanged cards from cache, rest fetched from backend.
    [[nodiscard]] QCoro::Task<WebDavSyncResult>
    fetchChangedContacts(const QString &collectionPath, const QList<WebDavItem> &items);

    /// Fetches all with single addressbook-query REPORT.
    /// Returns paths of all received cards.
    [[nodiscard]] QCoro::Task<std::expected<QSet<QString>, WebDavSyncResult>>
    queryContacts(const QString &collectionPath);

    [[nodiscard]] QCoro::Task<WebDavSyncResult> syncCollection(const QString &collectionPath,
                                                               const QString &requestedPath,
                                                               const QString &syncToken);

    [[nodiscard]] QCoro::Task<> processContactResults(const QList<WebDavItem> &results,
                                                      bool hrefsAreKnownChanged);
    [[nodiscard]] QCoro::Task<QStringList>
    readdUnchangedCachedContacts(const QList<WebDavItem> &items);
    [[nodiscard]] QCoro::Task<> readdCachedContacts(const QString &collectionPath,
                                                    const QSet<QString> &excludedPaths = {});

    /// Large loops hand control back to the event loop regularly to keep the UI responsive
    bool isTimeSliceExhausted() const;
    [[nodiscard]] QCoro::Task<> yieldToEventLoop();

    void pruneCachedContacts(const QString &collectionPath, const QSet<QString> &existingPaths);
    void pruneVanishedCollections(const QSet<QString> &knownCollectionPaths);

    void processVcard(QByteArray data, const QString &uuid, const QDateTime &modifiedDate);
    void loadCachedData(const size_t hash);
    void deleteOwnedCachedContacts();
    QString cacheFilePath(const size_t hash, bool createPath = false);
    void markCacheDirty();
    void flushCacheIfDirty();
    void flushCache();
    void processPhotoProperty(const QString &id, const QByteArray &data,
                              const QDateTime &modifiedDate) const;

    Contact::ContactSourceInfo sourceInfoFor(const QString &cardPath) const;

    void resetContacts();

    AddressBookManager *m_manager = nullptr;
    std::unique_ptr<WebDavClient> m_webdavClient;

    size_t m_settingsHash = 0;
    bool m_isCacheDirty = false;
    QElapsedTimer m_timeSliceTimer;
    QHash<QString, Contact *> m_cachedContacts;
    QHash<QString, QDateTime> m_ignoredIds;
    QHash<QString, QString> m_collectionNames;

    /// Normalized collection path -> DAV:sync-token
    QHash<QString, QString> m_syncTokens;

    QString m_group;
    BlockInfo m_blockInfo;

    CardDAVAddressBookFeederConfig m_config;
    QStringList m_sipStatusSubscriptableAttributes;

    int m_retryCount = 0;
    int m_retryInterval = 0;
};
