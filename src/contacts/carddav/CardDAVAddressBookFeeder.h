#pragma once

#include <QObject>
#include <IAddressBookFeeder.h>
#include <QtWebDAV/qwebdav.h>
#include <QtWebDAV/qwebdavdirparser.h>

#include "Contact.h"

class AddressBook;
class AddressBookManager;

class CardDAVAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit CardDAVAddressBookFeeder(const QString &group, AddressBookManager *parent = nullptr);

    void process() override;
    QUrl networkCheckURL() const override;

private Q_SLOTS:
    void onError(QString error) const;
    void onParserFinished();
    void flushCachImpl();

private:
    void init(const size_t settingsHash, const QString &host, const QString &path,
              const QString &user, const QString &password, int port, bool useSSL);
    void processImpl(const QString &password);

    void processVcard(QByteArray data, const QString &uuid, const QDateTime &modifiedDate);
    void loadCachedData(const size_t hash);
    QString cacheFilePath(const size_t hash, bool createPath = false);
    void processPhotoProperty(const QString &id, const QByteArray &data,
                              const QDateTime &modifiedDate) const;

    virtual void feedAddressBook();

    AddressBookManager *m_manager = nullptr;

    size_t m_settingsHash = 0;
    QWebdav m_webdav;
    QWebdavDirParser m_webdavParser;
    QTimer m_cacheWriteTimer;
    QHash<QString, Contact *> m_cachedContacts;
    QHash<QString, QDateTime> m_ignoredIds;

    QString m_group;
    bool m_block = false;
};
