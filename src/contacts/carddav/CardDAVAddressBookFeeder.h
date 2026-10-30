#pragma once

#include "CardDAVAddressBookFeederConfig.h"
#include "IAddressBookFeeder.h"
#include "Contact.h"
#include "BlockInfo.h"
#include "WebDavSyncResult.h"

#include <QObject>
#include <QSet>
#include <QCoroCore>

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

private Q_SLOTS:
    void flushCacheImpl();

private:
    void init();

    QCoro::Task<> run();
    [[nodiscard]] QCoro::Task<WebDavSyncResult>
    syncContactsCollection(const QString &collectionPath, int depth);
    /// If hrefsAreKnownChanged is true every card is parsed again (skipped cache heuristics)
    [[nodiscard]] QCoro::Task<WebDavSyncResult> fetchContacts(const QString &collectionPath,
                                                              const QStringList &hrefs,
                                                              bool hrefsAreKnownChanged = false);
    [[nodiscard]] QCoro::Task<WebDavSyncResult> syncCollection(const QString &collectionPath,
                                                               const QString &requestedPath,
                                                               const QString &syncToken);

    void readdCachedContacts(const QString &collectionPath,
                             const QSet<QString> &excludedPaths = {});
    void pruneCachedContacts(const QString &collectionPath, const QSet<QString> &existingPaths);
    void pruneVanishedCollections(const QSet<QString> &knownCollectionPaths);

    void processVcard(QByteArray data, const QString &uuid, const QDateTime &modifiedDate);
    void loadCachedData(const size_t hash);
    QString cacheFilePath(const size_t hash, bool createPath = false);
    void scheduleCacheWrite();
    void processPhotoProperty(const QString &id, const QByteArray &data,
                              const QDateTime &modifiedDate) const;

    void resetContacts();

    AddressBookManager *m_manager = nullptr;
    std::unique_ptr<WebDavClient> m_webdavClient;

    size_t m_settingsHash = 0;
    QTimer m_cacheWriteTimer;
    QHash<QString, Contact *> m_cachedContacts;
    QHash<QString, QDateTime> m_ignoredIds;

    /// Normalized collection path -> DAV:sync-token
    QHash<QString, QString> m_syncTokens;

    QString m_group;
    BlockInfo m_blockInfo;

    CardDAVAddressBookFeederConfig m_config;
    QStringList m_sipStatusSubscriptableAttributes;

    int m_retryCount = 0;
    int m_retryInterval = 0;

    bool m_areWebDavConnectionsInitialized = false;
};
