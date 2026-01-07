#include "AvatarManager.h"
#include "AddressBook.h"

#include <QPointer>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QMimeDatabase>

Q_LOGGING_CATEGORY(lcAvatarManager, "gonnect.app.AvatarManager")

using namespace std::chrono_literals;

AvatarManager::AvatarManager(QObject *parent) : QObject{ parent }
{
    const auto baseDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_avatarImageDirPath = QString("%1/avatars").arg(baseDirPath);

    QDir baseDir(baseDirPath);
    if (!baseDir.exists("avatars")) {
        baseDir.mkpath("avatars");
        qCInfo(lcAvatarManager) << "Created avatar image directory" << m_avatarImageDirPath;
    }

    m_updateContactsTimer.setSingleShot(true);
    m_updateContactsTimer.setInterval(5ms);
    m_updateContactsTimer.callOnTimeout(this, &AvatarManager::updateContacts);

    connect(&AddressBook::instance(), &AddressBook::contactAdded, this, [this](Contact *contact) {
        m_contactsWithPendingUpdates.append(QPointer(contact));

        if (!m_updateContactsTimer.isActive()) {
            m_updateContactsTimer.start();
        }
    });
    connect(&AddressBook::instance(), &AddressBook::contactModified, this,
            [this](Contact *contact) {
                m_contactsWithPendingUpdates.append(QPointer(contact));

                if (!m_updateContactsTimer.isActive()) {
                    m_updateContactsTimer.start();
                }
            });
}

void AvatarManager::updateContacts()
{
    if (m_contactsWithPendingUpdates.isEmpty()) {
        return;
    }

    const auto contactIds = readContactIdsFromDir();

    for (auto contactPtr : std::as_const(m_contactsWithPendingUpdates)) {
        if (contactPtr && contactIds.contains(contactPtr->id())) {
            contactPtr->setHasAvatar(true);
        }
    }

    m_contactsWithPendingUpdates.clear();
    Q_EMIT avatarsLoaded();
}

QList<const Contact *> AvatarManager::initialLoad()
{
    QList<const Contact *> dirtyContacts;
    QDir avatarDir(m_avatarImageDirPath);

    if (!avatarDir.isEmpty()) {
        // Do not load avatars from LDAP, but read existing avatars from directory
        auto &addressBook = AddressBook::instance();
        const auto dbTimes = readIdsFromDb();

        // Check which avatars are missing or outdated
        const auto contacts = addressBook.contacts();
        for (const Contact *contact : contacts) {
            const auto contactId = contact->id();
            if (dbTimes.contains(contactId) && contact->lastModified() > dbTimes.value(contactId)) {
                // Avatar of this contact is outdated
                dirtyContacts.append(contact);
            }
        }
    }

    Q_EMIT avatarsLoaded();

    return dirtyContacts;
}

QString AvatarManager::avatarPathFor(const QString &id)
{
    QString res = QString("%1/%2").arg(m_avatarImageDirPath, id);

    if (!res.isEmpty() && !QFile::exists(res)) {
        return "";
    }

    return res;
}

void AvatarManager::addExternalImage(const QString &id, const QByteArray &data,
                                     const QDateTime &modified)
{
    const auto dbModified = modifiedTimeInDb(id);

    if (dbModified.isValid() && dbModified < modified) {
        // Valid, but outdated DB timestamp; update
        createFile(id, data);
        updateAvatarModifiedTime(id, modified);
    } else if (dbModified.isNull()) {
        // Invalid/no DB timestamp found; create new entry
        createFile(id, data);
        QHash<QString, QDateTime> tmp;
        tmp.insert(id, modified);
        addIdsToDb(tmp);
    }

    if (auto contact = AddressBook::instance().lookupByContactId(id)) {
        contact->setHasAvatar(true);
    }

    Q_EMIT avatarAdded(id);
}

void AvatarManager::removeExternalImage(const QString &id)
{
    removeFile(id);
    QList<QString> tmp;
    tmp.append(id);
    removeIdsFromDb(tmp);

    if (auto contact = AddressBook::instance().lookupByContactId(id)) {
        contact->setHasAvatar(false);
    }

    Q_EMIT avatarRemoved(id);
}

void AvatarManager::createFile(const QString &id, const QByteArray &data) const
{
    QFile file(QString("%1/%2").arg(m_avatarImageDirPath, id));

    if (!file.open(QIODevice::WriteOnly)) {
        qCCritical(lcAvatarManager)
                << "Cannot open file" << QString("%1/%2").arg(m_avatarImageDirPath, id);
        return;
    }

    file.write(data);
}

void AvatarManager::removeFile(const QString &id) const
{
    QString file = QString("%1/%2").arg(m_avatarImageDirPath, id);

    if (!QFile::remove(file)) {
        qCCritical(lcAvatarManager)
                << "Cannot remove file" << QString("%1/%2").arg(m_avatarImageDirPath, id);
    }
}

void AvatarManager::addIdsToDb(QHash<QString, QDateTime> &idTimeMap) const
{
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcAvatarManager) << "Unable to open avatars databse:" << db.lastError().text();
    } else {
        qCInfo(lcAvatarManager) << "Successfully opened avatars database";

        qCInfo(lcAvatarManager) << "Writing new avatars item to database";
        QSqlQuery query(db);
        query.prepare("INSERT INTO avatars (id, lastModified) VALUES (:id, :lastModified);");

        QHashIterator it(idTimeMap);

        while (it.hasNext()) {
            it.next();

            query.bindValue(":id", it.key());
            query.bindValue(":lastModified", it.value().toSecsSinceEpoch());

            if (!query.exec()) {
                qCCritical(lcAvatarManager)
                        << "Error on executing SQL query:" << query.lastError().text();
            }
        }
    }
}

void AvatarManager::removeIdsFromDb(QList<QString> &idList) const
{
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcAvatarManager) << "Unable to open avatars databse:" << db.lastError().text();
    } else {
        qCInfo(lcAvatarManager) << "Successfully opened avatars database";

        qCInfo(lcAvatarManager) << "Removing avatars item from database";
        QSqlQuery query(db);
        query.prepare("DELETE FROM avatars WHERE id = :id;");

        for (const auto &id : idList) {
            query.bindValue(":id", id);

            if (!query.exec()) {
                qCCritical(lcAvatarManager)
                        << "Error on executing SQL query:" << query.lastError().text();
            }
        }
    }
}

void AvatarManager::updateAvatarModifiedTime(const QString &id, const QDateTime &modified) const
{
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcAvatarManager) << "Unable to open avatars database:" << db.lastError().text();
    } else {
        qCInfo(lcAvatarManager) << "Successfully opened avatars database";

        qCInfo(lcAvatarManager) << "Updating avatars item in database";
        QSqlQuery query(db);
        query.prepare("UPDATE avatars SET lastModified = :lastModified WHERE id = :id;");

        query.bindValue(":id", id);
        query.bindValue(":lastModified", modified.toSecsSinceEpoch());

        if (!query.exec()) {
            qCCritical(lcAvatarManager)
                    << "Error on executing SQL query:" << query.lastError().text();
        }
    }
}

QDateTime AvatarManager::modifiedTimeInDb(const QString &id) const
{
    auto db = QSqlDatabase::database();
    if (!db.open()) {
        qCCritical(lcAvatarManager) << "Unable to open avatars database:" << db.lastError().text();
        return QDateTime();
    } else {
        QSqlQuery query(db);
        query.prepare("SELECT lastModified FROM avatars WHERE id = :id;");
        query.bindValue(":id", id);

        if (!query.exec()) {
            qCCritical(lcAvatarManager)
                    << "Error on executing SQL query:" << query.lastError().text();
            return QDateTime();
        } else if (query.next()) {
            return QDateTime::fromSecsSinceEpoch(query.value("lastModified").toLongLong());
        }
    }

    return QDateTime();
}

QHash<QString, QDateTime> AvatarManager::readIdsFromDb() const
{
    QHash<QString, QDateTime> result;
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcAvatarManager) << "Unable to open avatars database:" << db.lastError().text();
    } else {
        qCInfo(lcAvatarManager) << "Successfully opened avatars database";

        qCInfo(lcAvatarManager) << "Reading avatar modified times from database";
        QSqlQuery query(db);
        query.prepare("SELECT * FROM avatars;");

        if (!query.exec()) {
            qCCritical(lcAvatarManager)
                    << "Error on executing SQL query:" << query.lastError().text();
        } else {
            while (query.next()) {
                result.insert(
                        query.value("id").toString(),
                        QDateTime::fromSecsSinceEpoch(query.value("lastModified").toLongLong()));
            }
        }

        qCInfo(lcAvatarManager) << "Read" << result.size() << "avatar modified times from database";
    }

    return result;
}

QStringList AvatarManager::readContactIdsFromDir() const
{
    QMimeDatabase db;

    QDir avatarDir(m_avatarImageDirPath);
    const auto fileList = avatarDir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot
                                              | QDir::NoSymLinks);

    QStringList resultList;

    for (const auto &fileName : fileList) {

        QMimeType mime = db.mimeTypeForFile(m_avatarImageDirPath + "/" + fileName);
        if (mime.inherits("image/jpeg") || mime.inherits("image/png")) {
            resultList.append(fileName);
        }
    }

    return resultList;
}
