#pragma once

#include <QObject>
#include <IAddressBookFeeder.h>
#include <qwebdav.h>
#include <qwebdavdirparser.h>

#include "Contact.h"

class AddressBook;

class CardDAVAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit CardDAVAddressBookFeeder(const size_t settingsHash, const QString &host,
                                      const QString &path, const QString &user,
                                      const QString &password, int port, bool useSSL,
                                      QObject *parent = nullptr);

    virtual void feedAddressBook(AddressBook &addressBook) override;

private slots:
    void onError(QString error) const;
    void onParserFinished();
    void flushCachImpl();

private:
    void processVcard(QByteArray data, const QString &uuid, const QDateTime &modifiedDate);
    void loadCachedData(const size_t hash);
    QString cacheFilePath(const size_t hash, bool createPath = false);
    void processPhotoProperty(const QString &id, const QByteArray &data,
                              const QDateTime &modifiedDate) const;

    size_t m_settingsHash = 0;
    QWebdav m_webdav;
    QWebdavDirParser m_webdavParser;
    QPointer<AddressBook> m_addressBook;
    QHash<QString, Contact *> m_cachedContacts;
    QTimer m_cacheWriteTimer;
    QHash<QString, QDateTime> m_ignoredIds;
};
