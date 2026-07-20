#pragma once

#include <QObject>
#include <IAddressBookFeeder.h>
#include "CardDAVAddressBookFeederConfig.h"
#include <QtWebDAV/qwebdav.h>
#include <QtWebDAV/qwebdavdirparser.h>

#include "Contact.h"
#include "BlockInfo.h"

class AddressBook;
class AddressBookManager;

class CardDAVAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit CardDAVAddressBookFeeder(const QString &group, const int retryCount,
                                      const int retryInterval,
                                      AddressBookManager *parent = nullptr);

    ~CardDAVAddressBookFeeder() = default;

    void process() override;
    QUrl networkCheckURL() const override;

private Q_SLOTS:
    void onError(QString error);
    void onParserFinished();
    void flushCacheImpl();

private:
    void init();
    void feedAddressBook(bool authFailed = false);

    void processVcard(QByteArray data, const QString &uuid, const QDateTime &modifiedDate);
    void loadCachedData(const size_t hash);
    QString cacheFilePath(const size_t hash, bool createPath = false);
    void processPhotoProperty(const QString &id, const QByteArray &data,
                              const QDateTime &modifiedDate) const;

    void checkErrorStatus();
    void resetContacts();

    AddressBookManager *m_manager = nullptr;

    size_t m_settingsHash = 0;
    QWebdav m_webdav;
    QWebdavDirParser m_webdavParser;
    QTimer m_cacheWriteTimer;
    QHash<QString, Contact *> m_cachedContacts;
    QHash<QString, QDateTime> m_ignoredIds;

    QString m_group;
    BlockInfo m_blockInfo;

    CardDAVAddressBookFeederConfig m_config;
    QStringList m_sipStatusSubscriptableAttributes;

    int m_retryCount = 0;
    int m_retryInterval = 0;
    int m_initialRetryCount = 0;

    bool m_pendingError = false;
    bool m_pendingAuth = false;
};
