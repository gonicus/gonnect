#include "AvatarManager.h"
#include "ErrorBus.h"
#include "AddressBook.h"

#include <ldap.h>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

Q_LOGGING_CATEGORY(lcAvatarManager, "gonnect.app.AvatarManager")

AvatarManager::AvatarManager(QObject *parent) : QObject{ parent }
{
    const auto baseDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_avatarImageDirPath = QString("%1/avatars").arg(baseDirPath);

    QDir baseDir(baseDirPath);
    if (!baseDir.exists("avatars")) {
        baseDir.mkpath("avatars");
        qCInfo(lcAvatarManager) << "Created avatar image directory" << m_avatarImageDirPath;
    }
}

void AvatarManager::initialLoad(const QString &ldapUrl, const QString &ldapBase,
                                const QString &ldapFilter)
{
    QDir avatarDir(m_avatarImageDirPath);
    if (avatarDir.isEmpty()) {
        qCInfo(lcAvatarManager)
                << "Avatar image directory is empty - triggering initial load of all images";
        loadAll(ldapUrl, ldapBase, ldapFilter);
    } else {
        // Do not load avatars from LDAP, but read existing avatars from directory
        auto &addressBook = AddressBook::instance();
        const auto dbTimes = readIdsFromDb();
        QList<const Contact *> dirtyContacts;

        // Check which avatars are missing or outdated
        const auto contacts = addressBook.contacts();
        for (const Contact *contact : contacts) {
            const auto contactId = contact->id();
            if (dbTimes.contains(contactId) && contact->lastModified() > dbTimes.value(contactId)) {
                // Avatar of this contact is outdated
                dirtyContacts.append(contact);
            }
        }

        if (dirtyContacts.size()) {
            loadAvatars(dirtyContacts, ldapUrl, ldapBase, ldapFilter);

            for (const Contact *contact : std::as_const(dirtyContacts)) {
                updateAvatarModifiedTime(contact->id(), contact->lastModified());
            }
        }

        // Read from directory
        const auto contactIds = readContactIdsFromDir();

        for (const auto &contactId : contactIds) {
            if (auto contact = addressBook.lookupByContactId(contactId)) {
                contact->setHasAvatar(true);
            } else {
                qCCritical(lcAvatarManager) << "Found avatar for contact id" << contactId
                                            << "but no contact for it - ignoring";
            }
        }
    }

    emit avatarsLoaded();
}

QString AvatarManager::avatarPathFor(const QString &id)
{
    return QString("%1/%2.jpg").arg(m_avatarImageDirPath, id);
}

void AvatarManager::clearCStringlist(char **attrs) const
{
    char **p;

    for (p = attrs; *p; p++) {
        free(*p);
    }

    free(attrs);
}

void AvatarManager::createFile(const QString &id, const QByteArray &data) const
{
    QFile file(QString("%1/%2.jpg").arg(m_avatarImageDirPath, id));

    if (!file.open(QIODevice::WriteOnly)) {
        qCCritical(lcAvatarManager)
                << "Cannot open file" << QString("%1/%2.jpg").arg(m_avatarImageDirPath, id);
        return;
    }

    file.write(data);
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

void AvatarManager::updateAvatarModifiedTime(const QString &id, const QDateTime &modified) const
{
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcAvatarManager) << "Unable to open avatars databse:" << db.lastError().text();
    } else {
        qCInfo(lcAvatarManager) << "Successfully opened avatars database";

        qCInfo(lcAvatarManager) << "Writing new avatars item to database";
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

QHash<QString, QDateTime> AvatarManager::readIdsFromDb() const
{
    QHash<QString, QDateTime> result;
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcAvatarManager) << "Unable to open avatars databse:" << db.lastError().text();
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

void AvatarManager::loadAvatars(const QList<const Contact *> &contacts, const QString &ldapUrl,
                                const QString &ldapBase, const QString &ldapFilter)
{
    QStringList filterList;
    filterList.reserve(contacts.size());

    for (const Contact *contact : contacts) {
        filterList.append(QString("(cn=%1)").arg(contact->name()));
    }

    const auto filter = QString("(& %1 (| %2))").arg(ldapFilter, filterList.join(' '));
    loadAll(ldapUrl, ldapBase, filter);
}

QStringList AvatarManager::readContactIdsFromDir() const
{
    QDir avatarDir(m_avatarImageDirPath);
    const auto fileList = avatarDir.entryList(
            { "*.jpg" }, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    QStringList resultList;

    for (const auto &fileInfo : fileList) {
        const auto fileName = fileInfo;

        if (fileName.length() == 64 + 4) { // File name scheme: uuid (64 chars) + ".jpg"
            resultList.append(fileName.left(64));
        }
    }

    return resultList;
}

void AvatarManager::loadAll(const QString &ldapUrl, const QString &ldapBase,
                            const QString &ldapFilter)
{
    char *a = nullptr;
    char *dnTemp = nullptr;
    BerElement *ber = nullptr;
    struct berval **vals;
    char *matchedMsg = nullptr;
    char *errorMsg = nullptr;
    int numEntries = 0, numRefs = 0, result = 0, msgType = 0, parseResultCode = 0;
    LDAP *ldap = nullptr;
    LDAPMessage *msg = nullptr;

    QStringList attributes = { "cn", "jpegPhoto", "modifyTimestamp" };

    size_t i = 0;
    char **attrs = (char **)malloc((attributes.count() + 1) * sizeof(char *));
    for (auto &attr : std::as_const(attributes)) {
        size_t sz = attr.size() + 1;
        char *p = (char *)malloc(sz);
        strncpy(p, attr.toLocal8Bit().toStdString().c_str(), sz);
        attrs[i++] = p;
    }
    attrs[i] = NULL;

    QString dn;
    QDateTime modifyTimestamp;
    QByteArray jpegPhoto;
    QHash<QString, QDateTime> ids;
    int count = 0;

    qCInfo(lcAvatarManager) << "Connecting to LDAP service";
    qCInfo(lcAvatarManager) << "LDAP url:" << ldapUrl << "base:" << ldapBase
                            << "ldapFilter:" << ldapFilter;

    result = ldap_initialize(&ldap, ldapUrl.toStdString().c_str());
    if (result != LDAP_SUCCESS) {
        qCCritical(lcAvatarManager)
                << "Could not initialize LDAP handle from uri:" << ldap_err2string(result);
        ErrorBus::instance().addError(
                tr("Failed to initialize LDAP connection: %1").arg(ldap_err2string(result)));
        clearCStringlist(attrs);
        return;
    }

    result = ldap_search_ext_s(ldap, ldapBase.toLocal8Bit().data(), LDAP_SCOPE_SUBTREE,
                               ldapFilter.toStdString().c_str(), attrs, false, NULL, NULL, NULL,
                               LDAP_NO_LIMIT, &msg);

    clearCStringlist(attrs);

    if (result != LDAP_SUCCESS) {
        qCCritical(lcAvatarManager) << "Error on search request: " << ldap_err2string(result);
        ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(result)));
        return;
    }

    numEntries = ldap_count_entries(ldap, msg);
    numRefs = ldap_count_references(ldap, msg);

    qCInfo(lcAvatarManager) << "Retrieved" << numEntries << "entries and" << numRefs << "refs";

    for (msg = ldap_first_message(ldap, msg); msg != NULL; msg = ldap_next_message(ldap, msg)) {

        msgType = ldap_msgtype(msg);

        switch (msgType) {
        case LDAP_RES_SEARCH_ENTRY: {
            dn = "";
            modifyTimestamp = QDateTime();
            jpegPhoto = "";

            // Iterate over attributes
            for (a = ldap_first_attribute(ldap, msg, &ber); a != NULL;
                 a = ldap_next_attribute(ldap, msg, ber)) {

                // Get DN
                if ((dnTemp = ldap_get_dn(ldap, msg)) != NULL) {
                    dn = dnTemp;
                    ldap_memfree(dnTemp);
                }

                // Iterate over values
                if ((vals = ldap_get_values_len(ldap, msg, a)) != NULL) {

                    char *val = (**vals).bv_val;

                    if (strcmp(a, "jpegPhoto") == 0) {
                        for (uint i = 0; i < (**vals).bv_len; ++i) {
                            jpegPhoto.append(*val);
                            val += sizeof(char);
                        }
                    } else if (strcmp(a, "modifyTimestamp") == 0) {
                        modifyTimestamp = QDateTime::fromString(val, "yyyyMMddhhmmsst");
                    }

                    ldap_value_free_len(vals);
                }

                ldap_memfree(a);
            }

            if (!jpegPhoto.isEmpty()) {
                const auto contactId = AddressBook::instance().hashifyCn(dn);

                auto contact = AddressBook::instance().lookupByContactId(contactId);
                if (!contact) {
                    qCCritical(lcAvatarManager)
                            << "Found avatar image but not contact for" << dn << contactId;
                    break;
                }
                contact->setHasAvatar(true);
                createFile(contactId, jpegPhoto);
                ids.insert(contactId, modifyTimestamp);
                ++count;
            }

            a = nullptr;

            ber_free(ber, 0);
            ber = nullptr;

            break;
        }

        case LDAP_RES_SEARCH_RESULT: {
            parseResultCode =
                    ldap_parse_result(ldap, msg, &result, &matchedMsg, &errorMsg, NULL, NULL, 1);
            if (parseResultCode != LDAP_SUCCESS) {
                qCCritical(lcAvatarManager)
                        << "LDAP parse error:" << ldap_err2string(parseResultCode);
                ErrorBus::instance().addError(
                        tr("Parse error: %1").arg(ldap_err2string(parseResultCode)));
                return;
            }
            break;
        }

        default:
            qCCritical(lcAvatarManager) << "Unknown message type:" << msgType;
            return;
        }
    }

    addIdsToDb(ids);

    qCInfo(lcAvatarManager) << "Loaded" << count << "avatars";

    ldap_unbind_ext(ldap, NULL, NULL);
}
