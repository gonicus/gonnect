#include "CardDAVAddressBookFeeder.h"
#include "AddressBook.h"
#include "AvatarManager.h"

#include <QRegularExpression>
#include <QLoggingCategory>
#include <QImage>

#include <vcard.h>
#include <text_io.h>

#define CARDDAV_MAGIC 0x0891
#define CARDDAV_VERSION 0x00

Q_LOGGING_CATEGORY(lcCardDAVAddressBookFeeder, "gonnect.app.feeder.CardDAVAddressBookFeeder")

using namespace std::chrono_literals;

CardDAVAddressBookFeeder::CardDAVAddressBookFeeder(const size_t settingsHash, const QString &host,
                                                   const QString &path, const QString &user,
                                                   const QString &password, int port, bool useSSL,
                                                   QObject *parent)
    : QObject{ parent }, m_settingsHash{ settingsHash }
{
    m_cacheWriteTimer.setSingleShot(true);
    m_cacheWriteTimer.setInterval(3s);
    m_cacheWriteTimer.callOnTimeout(this, &CardDAVAddressBookFeeder::flushCachImpl);

    loadCachedData(settingsHash);

    m_webdav.setConnectionSettings(useSSL ? QWebdav::HTTPS : QWebdav::HTTP, host, path, user,
                                   password, port);
    connect(&m_webdavParser, &QWebdavDirParser::finished, this,
            &CardDAVAddressBookFeeder::onParserFinished);
    connect(&m_webdavParser, &QWebdavDirParser::errorChanged, this,
            &CardDAVAddressBookFeeder::onError);
    connect(&m_webdav, &QWebdav::errorChanged, this, &CardDAVAddressBookFeeder::onError);
}

void CardDAVAddressBookFeeder::feedAddressBook(AddressBook &addressBook)
{
    m_addressBook = QPointer(&addressBook);
    m_webdavParser.listDirectory(&m_webdav, "/");
}

void CardDAVAddressBookFeeder::processVcard(QByteArray data, const QString &uuid,
                                            const QDateTime &modifiedDate)
{
    Q_ASSERT(!m_addressBook.isNull());

    std::istringstream stringStream(data.toStdString());
    TextReader reader(stringStream);
    auto cards = reader.parseCards();

    for (auto &card : cards) {
        auto &props = card.properties();

        QString uid;
        QString name;
        QString org;
        QString email;
        QByteArray photoData;
        QList<Contact::PhoneNumber> phoneNumbers;

        for (auto &prop : props) {
            const auto propName = prop.getName();

            if (propName == "UID") {
                uid = QString::fromStdString(prop.getValue());
            } else if (propName == "FN") {
                name = QString::fromStdString(prop.getValue());
            } else if (propName == "ORG") {
                org = QString::fromStdString(prop.getValue());
            } else if (propName == "EMAIL") {
                email = QString::fromStdString(prop.getValue());
            } else if (propName == "TEL") {
                phoneNumbers.append({ Contact::NumberType::Unknown,
                                      QString::fromStdString(prop.getValue()), false });
            } else if (propName == "PHOTO" || propName.starts_with("PHOTO:data:image/jpeg")) {
                photoData = QByteArray::fromStdString(prop.getValue());
            }
        }

        if (!uuid.isEmpty() && !name.isEmpty() && !phoneNumbers.isEmpty()) {
            Contact *contact =
                    m_addressBook->addContact(uid, name, org, email, modifiedDate, phoneNumbers);
            m_cachedContacts.insert(uuid, contact);

            processPhotoProperty(uid, photoData, modifiedDate);

        } else if (!uuid.isEmpty()) {
            m_ignoredIds.insert(uuid, modifiedDate);
        }
    }

    if (!m_cacheWriteTimer.isActive()) {
        m_cacheWriteTimer.start();
    }
}

void CardDAVAddressBookFeeder::loadCachedData(const size_t hash)
{
    if (!hash) {
        return;
    }

    const auto filePath = cacheFilePath(hash);
    QFile cacheFile(filePath);

    if (!cacheFile.exists()) {
        return;
    }

    if (!cacheFile.open(QIODevice::ReadOnly)) {
        onError(QString("Unable to open file %1").arg(filePath));
        return;
    }

    QDataStream in(&cacheFile);

    quint16 magic;
    quint8 version;
    qsizetype numberOfContacts;

    in >> magic;
    in >> version;
    in >> m_ignoredIds;
    in >> numberOfContacts;

    if (magic != CARDDAV_MAGIC || version != CARDDAV_VERSION) {
        qCInfo(lcCardDAVAddressBookFeeder) << "CardDAV cache file at" << filePath
                                           << "in invalid and will therefore be removed.";
        cacheFile.remove();
        return;
    }

    for (qsizetype i = 0; i < numberOfContacts; ++i) {
        QString key;
        Contact *contact = new Contact(this);
        in >> key;
        in >> *contact;
        m_cachedContacts.insert(key, contact);
    }

    cacheFile.close();

    qCInfo(lcCardDAVAddressBookFeeder)
            << "Loaded" << m_cachedContacts.size() << "contacts from cache with hash" << hash;
}

QString CardDAVAddressBookFeeder::cacheFilePath(const size_t hash, bool createPath)
{
    const QString path =
            QString("%1/cache/carddav")
                    .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    if (createPath) {
        QDir dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        dir.mkpath(path);
    }

    return QString("%1/%2.dat").arg(path).arg(hash);
}

void CardDAVAddressBookFeeder::processPhotoProperty(const QString &id, const QByteArray &data,
                                                    const QDateTime &modifiedDate) const
{

    // Detect vCard version
    // 2.1: PHOTO;JPEG;ENCODING=BASE64:[base64-data]
    // 3.0: PHOTO;TYPE=JPEG;ENCODING=b:[base64-data]
    // 4.0: PHOTO:data:image/jpeg;base64,[base64-data]

    const auto splitted = data.split(':');
    if (splitted.size() != 2) {
        return;
    }

    // Checking vCard 2.1 and 3.0
    const auto head = splitted.at(0).split(';');
    QByteArray base64Str;
    bool isJpeg = false;
    bool isBase64 = false;

    for (const auto &part : head) {
        if (part == "TYPE=JPEG" || part == "JPEG") {
            isJpeg = true;
        } else if (part == "ENCODING=b" || part == "ENCODING=BASE64") {
            isBase64 = true;
        }
    }

    if (isJpeg && isBase64) {
        base64Str = splitted.at(1);
    } else if (data.startsWith("base64,")) {
        // Must be vCard 4.0
        base64Str = data.sliced(7);
    }

    if (base64Str.isEmpty()) {
        return;
    }

    // Convert base64 data to image
    const QByteArray decoded = QByteArray::fromBase64(base64Str);
    AvatarManager::instance().addExternalImage(id, decoded, modifiedDate);
}

void CardDAVAddressBookFeeder::onError(QString error) const
{
    qCCritical(lcCardDAVAddressBookFeeder) << "Error:" << error;
}

void CardDAVAddressBookFeeder::onParserFinished()
{
    const auto list = m_webdavParser.getList();
    for (const auto &item : list) {
        const auto cacheId = item.name();
        const auto modifiedDate = item.lastModified();

        if (m_ignoredIds.contains(cacheId) && m_ignoredIds.value(cacheId) >= modifiedDate) {
            continue;
        } else if (Contact *cashedContact = m_cachedContacts.value(cacheId, nullptr)) {
            if (cashedContact->lastModified() >= modifiedDate) {
                m_addressBook->addContact(cashedContact);
                continue;
            }
        }

        QNetworkReply *reply = m_webdav.get(item.path());
        connect(
                reply, &QNetworkReply::readyRead, this,
                [reply, cacheId, modifiedDate, this]() {
                    if (!reply || !m_addressBook) {
                        return;
                    }
                    QByteArray data = reply->readAll();
                    reply->deleteLater();

                    QMimeDatabase db;
                    QMimeType type = db.mimeTypeForData(data);
                    if (type.name() == "text/vcard") {
                        processVcard(data, cacheId, modifiedDate);
                    }
                },
                Qt::ConnectionType::SingleShotConnection);
    }
}

void CardDAVAddressBookFeeder::flushCachImpl()
{
    const auto filePath = cacheFilePath(m_settingsHash, true);
    QFile cacheFile(filePath);

    if (!cacheFile.open(QIODevice::WriteOnly)) {
        onError(QString("Unable to open file %1").arg(filePath));
        return;
    }

    QDataStream out(&cacheFile);
    out << quint16(CARDDAV_MAGIC);
    out << quint8(CARDDAV_VERSION);
    out << m_ignoredIds;
    out << m_cachedContacts.size();

    QHashIterator it(std::as_const(m_cachedContacts));
    while (it.hasNext()) {
        it.next();
        out << it.key() << *(it.value());
    }

    cacheFile.close();

    qCInfo(lcCardDAVAddressBookFeeder)
            << m_cachedContacts.size() << "contacts written to CardDAV cache with hash"
            << m_settingsHash;
}

#undef CARDDAV_MAGIC
#undef CARDDAV_VERSION
