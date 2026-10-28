#pragma once

#include "CardDAVAddressBookFeederConfig.h"
#include "IAddressBookFeeder.h"
#include "Contact.h"
#include "BlockInfo.h"

#include <QObject>
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

    void processVcard(QByteArray data, const QString &uuid, const QDateTime &modifiedDate);
    void loadCachedData(const size_t hash);
    QString cacheFilePath(const size_t hash, bool createPath = false);
    void processPhotoProperty(const QString &id, const QByteArray &data,
                              const QDateTime &modifiedDate) const;

    void resetContacts();

    AddressBookManager *m_manager = nullptr;
    std::unique_ptr<WebDavClient> m_webdavClient;

    size_t m_settingsHash = 0;
    QTimer m_cacheWriteTimer;
    QHash<QString, Contact *> m_cachedContacts;
    QHash<QString, QDateTime> m_ignoredIds;

    QString m_group;
    BlockInfo m_blockInfo;

    CardDAVAddressBookFeederConfig m_config;
    QStringList m_sipStatusSubscriptableAttributes;

    int m_retryCount = 0;
    int m_retryInterval = 0;

    bool m_areWebDavConnectionsInitialized = false;
};
