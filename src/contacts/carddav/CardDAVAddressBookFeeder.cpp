#include "CardDAVAddressBookFeeder.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "AvatarManager.h"
#include "ReadOnlyConfdSettings.h"
#include "PhoneNumberUtil.h"
#include "ErrorBus.h"
#include "SecretResponse.h"
#include "WebDavClient.h"

#include <QRegularExpression>
#include <QLoggingCategory>
#include <QImage>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include <vcard.h>
#include <text_io.h>

#define CARDDAV_MAGIC 0x0891
#define CARDDAV_VERSION 0x02

Q_LOGGING_CATEGORY(lcCardDAVAddressBookFeeder, "gonnect.app.feeder.CardDAVAddressBookFeeder")

using namespace std::chrono_literals;

CardDAVAddressBookFeeder::CardDAVAddressBookFeeder(const QString &group, const int retryCount,
                                                   const int retryInterval,
                                                   AddressBookManager *parent)
    : QObject(parent), m_group(group), m_retryCount(retryCount), m_retryInterval(retryInterval)
{
    m_manager = qobject_cast<AddressBookManager *>(parent);
}

// Defined here so WebDavClient can be used as forward declaration in header file
CardDAVAddressBookFeeder::~CardDAVAddressBookFeeder() = default;

void CardDAVAddressBookFeeder::init()
{
    if (!m_areWebDavConnectionsInitialized) {
        m_areWebDavConnectionsInitialized = true;

        m_cacheWriteTimer.setSingleShot(true);
        m_cacheWriteTimer.setInterval(3s);
        m_cacheWriteTimer.callOnTimeout(this, &CardDAVAddressBookFeeder::flushCacheImpl);
    }

    loadCachedData(m_settingsHash);
}

void CardDAVAddressBookFeeder::resetContacts()
{
    AddressBook::instance().removeContactsBySource(m_group);
}

QCoro::Task<> CardDAVAddressBookFeeder::run()
{
    m_isProcessing = true;
    bool forcePrompt = false;
    int retriesLeft = m_retryCount;

    while (true) {

        // Retrieve secret
        const auto secret = co_await m_manager->acquireSecretAsync(forcePrompt, m_group);
        if (secret.hasError) {
            ErrorBus::instance().addError(tr("Authentication error for %1").arg(m_group));
            m_isProcessing = false;
            co_return;
        }

        // Retrieve WebDAV root object(s)
        const QUrl webdavUrl(QString("%1://%2:%3%4")
                                     .arg(m_config.useSSL ? "https" : "http", m_config.host,
                                          QString::number(m_config.port), m_config.path));

        m_webdavClient = std::make_unique<WebDavClient>(
                WebDavClient::ConnectionConfig{ webdavUrl, m_config.user, secret.secret });

        qCDebug(lcCardDAVAddressBookFeeder) << "Created WebDavClient with url" << webdavUrl;

        resetContacts();

        const auto result = co_await syncContactsCollection("", 0);

        if (result == WebDavSyncResult::AuthRequired) {
            qCWarning(lcCardDAVAddressBookFeeder)
                    << "Failed to process CardDAV source: invalid password";
            forcePrompt = true;
            continue;
        }
        if (result == WebDavSyncResult::OtherError) {
            if (retriesLeft-- <= 0) {
                ErrorBus::instance().addError(
                        tr("CardDAV source %1 repeatedly failed - retries exhausted").arg(m_group));
                m_isProcessing = false;
                co_return;
            }

            qCWarning(lcCardDAVAddressBookFeeder)
                    << "Failed to process CardDAV sources" << m_group << "- trying again after"
                    << m_retryInterval << "ms," << retriesLeft << "retries left";
            co_await QCoro::sleepFor(std::chrono::milliseconds(m_retryInterval));
            continue;
        }

        break;
    }

    m_isProcessing = false;
}

QCoro::Task<WebDavSyncResult>
CardDAVAddressBookFeeder::syncContactsCollection(const QString &collectionPath, int depth)
{
    auto *reply = co_await m_webdavClient->propfind(collectionPath, 1,
                                                    { { "D", "DAV:", "resourcetype" },
                                                      { "D", "DAV:", "getetag" },
                                                      { "D", "DAV:", "getlastmodified" },
                                                      { "D", "DAV:", "sync-token" } });

    if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
        reply->deleteLater();
        co_return WebDavSyncResult::AuthRequired;
    } else if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        qCWarning(lcCardDAVAddressBookFeeder)
                << "PROPFIND failed for" << collectionPath << ":" << reply->errorString();
        co_return WebDavSyncResult::OtherError;
    }

    const QUrl requestedUrl = reply->url();
    const QString requestedPath = WebDavItem::normalizedPath(requestedUrl);
    const auto items = WebDavPropfindParser{}.parseMultiStatus(reply->readAll());
    reply->deleteLater();

    // Check if sync-collection is supported (i.e. sync-token not empty)
    QString collectionSyncToken;
    for (const auto &item : items) {
        if (item.isContactCollection() && item.normalizedPath() == requestedPath) {
            collectionSyncToken = item.syncToken();

            if (collectionSyncToken.isEmpty()) {
                qCDebug(lcCardDAVAddressBookFeeder) << "No sync-token for collection"
                                                    << item.normalizedPath() << "- using multiget";
            } else {
                qCDebug(lcCardDAVAddressBookFeeder)
                        << "Found sync-token for collection" << item.normalizedPath()
                        << "- sync-collection supported";
            }
            break;
        }
    }

    // Gather hrefs
    QStringList hrefsToFetch;
    QSet<QString> normalizedHrefs;
    for (const auto &item : items) {
        if (!item.isCollection()) {
            hrefsToFetch.append(item.href);
            normalizedHrefs.insert(item.normalizedPath());
        }
    }

    const auto storedSyncToken = m_syncTokens.value(requestedPath, "");
    bool needsFullSync = collectionSyncToken.isEmpty() || storedSyncToken.isEmpty();
    WebDavSyncResult result = WebDavSyncResult::Success;

    if (!needsFullSync && storedSyncToken == collectionSyncToken) {
        qCDebug(lcCardDAVAddressBookFeeder) << "Collection" << requestedPath << "is unchanged";
        readdCachedContacts(requestedPath);

    } else if (!needsFullSync) {
        result = co_await syncCollection(collectionPath, requestedPath, storedSyncToken);

        if (result == WebDavSyncResult::InvalidSyncToken) {
            qCInfo(lcCardDAVAddressBookFeeder)
                    << "Sync token for" << requestedPath << "was rejected - doing full sync";
            m_syncTokens.remove(requestedPath);
            needsFullSync = true;
        }
    }

    if (needsFullSync) {
        pruneCachedContacts(requestedPath, normalizedHrefs);
        result = co_await fetchContacts(collectionPath, hrefsToFetch);
    }

    if (result == WebDavSyncResult::AuthRequired) {
        co_return WebDavSyncResult::AuthRequired;
    }

    bool anyBatchFailed = result != WebDavSyncResult::Success;

    // Update sync token
    if (!anyBatchFailed) {
        const bool tokenChanged = collectionSyncToken.isEmpty()
                ? m_syncTokens.remove(requestedPath)
                : m_syncTokens.value(requestedPath) != collectionSyncToken;

        if (tokenChanged) {
            if (!collectionSyncToken.isEmpty()) {
                m_syncTokens.insert(requestedPath, collectionSyncToken);
            }

            qCDebug(lcCardDAVAddressBookFeeder)
                    << "Sync token for" << requestedPath << "is now" << collectionSyncToken;
            scheduleCacheWrite();
        }
    }

    // Descent one level in tree
    if (depth == 0) {
        QSet<QString> knownCollectionPaths = { requestedPath };

        for (const auto &item : items) {
            if (item.normalizedPath() == requestedPath) {
                continue; // Self entry - already processed
            }

            if (item.isCollection() && item.isContactCollection()) {
                knownCollectionPaths.insert(item.normalizedPath());

                const auto childResult = co_await syncContactsCollection(item.href, depth + 1);
                if (childResult == WebDavSyncResult::AuthRequired) {
                    co_return WebDavSyncResult::AuthRequired;
                }
                if (childResult == WebDavSyncResult::OtherError) {
                    anyBatchFailed = true;
                }
            }
        }

        pruneVanishedCollections(knownCollectionPaths);
    }

    co_return anyBatchFailed ? WebDavSyncResult::OtherError : WebDavSyncResult::Success;
}

QCoro::Task<WebDavSyncResult> CardDAVAddressBookFeeder::fetchContacts(const QString &collectionPath,
                                                                      const QStringList &hrefs,
                                                                      bool hrefsAreKnownChanged)
{
    bool anyBatchFailed = false;

    // Retrieve cards in batches via multiget
    const auto batchSize = m_config.multigetBatchSize;

    for (qsizetype offset = 0; offset < hrefs.size(); offset += batchSize) {
        const auto batch = hrefs.mid(offset, batchSize);

        const auto body = WebDavPropfindParser{}.buildMultigetBody(
                "CARD", "urn:ietf:params:xml:ns:carddav", "addressbook-multiget",
                { { "D", "DAV:", "getetag" },
                  { "D", "DAV:", "getlastmodified" },
                  { "CARD", "urn:ietf:params:xml:ns:carddav", "address-data" } },
                batch);

        auto *multigetReply = co_await m_webdavClient->report(collectionPath, body);
        if (multigetReply->error() == QNetworkReply::AuthenticationRequiredError) {
            multigetReply->deleteLater();
            co_return WebDavSyncResult::AuthRequired;
        }
        if (multigetReply->error() != QNetworkReply::NoError) {
            multigetReply->deleteLater();
            qCWarning(lcCardDAVAddressBookFeeder)
                    << "addressbook-multiget batch failed:" << multigetReply->errorString();
            anyBatchFailed = true;
            continue;
        }

        // Process response
        const auto results = WebDavPropfindParser{}.parseMultiStatus(multigetReply->readAll());
        multigetReply->deleteLater();

        qCDebug(lcCardDAVAddressBookFeeder) << "Received addressbook-multiget response" << offset
                                            << "to" << std::min(offset + batchSize, hrefs.size());

        for (const auto &result : results) {
            const auto contactData =
                    result.property("urn:ietf:params:xml:ns:carddav", "address-data").text.toUtf8();
            if (contactData.isEmpty()) {
                continue;
            }

            const auto cacheId = result.normalizedPath();
            const auto modifiedDate = result.lastModified();

            if (!hrefsAreKnownChanged) {
                if (m_ignoredIds.contains(cacheId) && m_ignoredIds.value(cacheId) >= modifiedDate) {
                    continue;
                }

                if (Contact *cachedContact = m_cachedContacts.value(cacheId, nullptr)) {
                    if (cachedContact->lastModified() >= modifiedDate) {
                        AddressBook::instance().addContact(cachedContact);
                        continue;
                    }
                }
            }

            processVcard(contactData, cacheId, modifiedDate);
        }
    }

    co_return anyBatchFailed ? WebDavSyncResult::OtherError : WebDavSyncResult::Success;
}

QCoro::Task<WebDavSyncResult>
CardDAVAddressBookFeeder::syncCollection(const QString &collectionPath,
                                         const QString &requestedPath, const QString &syncToken)
{
    const auto changes = co_await m_webdavClient->syncCollection(collectionPath, syncToken);
    if (changes.result != WebDavSyncResult::Success) {
        co_return changes.result;
    }

    qCDebug(lcCardDAVAddressBookFeeder)
            << "sync-collection for" << requestedPath << "reported" << changes.changedHrefs.size()
            << "changed and" << changes.removedPaths.size() << "removed cards";

    // Removed cards
    m_cachedContacts.removeIf([&changes](QHash<QString, Contact *>::iterator it) {
        if (!changes.removedPaths.contains(it.key())) {
            return false;
        }
        it.value()->deleteLater();
        return true;
    });

    m_ignoredIds.removeIf([&changes](QHash<QString, QDateTime>::iterator it) {
        return changes.removedPaths.contains(it.key());
    });

    if (!changes.removedPaths.isEmpty()) {
        scheduleCacheWrite();
    }

    // Cache -> unchanged cards; fetch -> changed cards
    const auto changedPaths =
            QSet<QString>(changes.changedHrefs.keyBegin(), changes.changedHrefs.keyEnd());
    readdCachedContacts(requestedPath, changedPaths);

    co_return co_await fetchContacts(collectionPath, changes.changedHrefs.values(), true);
}

void CardDAVAddressBookFeeder::readdCachedContacts(const QString &collectionPath,
                                                   const QSet<QString> &excludedPaths)
{
    QHashIterator it(m_cachedContacts);
    while (it.hasNext()) {
        it.next();
        if (WebDavItem::collectionPathOf(it.key()) == collectionPath
            && !excludedPaths.contains(it.key())) {
            AddressBook::instance().addContact(it.value());
        }
    }
}

void CardDAVAddressBookFeeder::pruneCachedContacts(const QString &collectionPath,
                                                   const QSet<QString> &existingPaths)
{
    const auto isStale = [collectionPath, existingPaths](const QString &path) {
        return WebDavItem::collectionPathOf(path) == collectionPath
                && !existingPaths.contains(path);
    };

    const auto removedContacts =
            m_cachedContacts.removeIf([isStale](QHash<QString, Contact *>::iterator it) {
                if (!isStale(it.key())) {
                    return false;
                }
                it.value()->deleteLater();
                return true;
            });

    const auto removedIgnoredIds = m_ignoredIds.removeIf(
            [isStale](QHash<QString, QDateTime>::iterator it) { return isStale(it.key()); });

    if (removedContacts || removedIgnoredIds) {
        qCDebug(lcCardDAVAddressBookFeeder)
                << "Pruned" << removedContacts << "contacts and" << removedIgnoredIds
                << "ignored ids from cache for" << collectionPath;

        scheduleCacheWrite();
    }
}

void CardDAVAddressBookFeeder::pruneVanishedCollections(const QSet<QString> &knownCollectionPaths)
{
    const auto isVanished = [knownCollectionPaths](const QString &path) {
        return !knownCollectionPaths.contains(WebDavItem::collectionPathOf(path));
    };

    const auto removedContacts =
            m_cachedContacts.removeIf([isVanished](QHash<QString, Contact *>::iterator it) {
                if (!isVanished(it.key())) {
                    return false;
                }
                it.value()->deleteLater();
                return true;
            });

    const auto removedIgnoredIds = m_ignoredIds.removeIf(
            [isVanished](QHash<QString, QDateTime>::iterator it) { return isVanished(it.key()); });

    const auto removedSyncTokens =
            m_syncTokens.removeIf([knownCollectionPaths](QHash<QString, QString>::iterator it) {
                return !knownCollectionPaths.contains(it.key());
            });

    if (removedContacts || removedIgnoredIds || removedSyncTokens) {
        qCDebug(lcCardDAVAddressBookFeeder)
                << "Pruned vanished collections:" << removedContacts << "contacts,"
                << removedIgnoredIds << "ignored ids," << removedSyncTokens << "sync tokens";

        scheduleCacheWrite();
    }
}

void CardDAVAddressBookFeeder::processVcard(QByteArray data, const QString &uuid,
                                            const QDateTime &modifiedDate)
{
    // Check vCard version
    static const QStringList supportedVersions = { "3", "4" };
    static const QRegularExpression versionRegExp("VERSION:(?<version>[0-9.]+?)(\\r\\n?|\\n)");

    const auto matchResult = versionRegExp.match(data);
    if (!matchResult.hasMatch()) {
        qCCritical(lcCardDAVAddressBookFeeder) << "Cannot parse version from vCard - ignoring";
        return;
    }

    const auto versionStr = matchResult.captured("version");
    const auto versionSplit = versionStr.split('.', Qt::SkipEmptyParts);
    if (versionSplit.isEmpty()) {
        qCCritical(lcCardDAVAddressBookFeeder) << "Cannot split version string" << versionStr;
        return;
    }

    if (!supportedVersions.contains(versionSplit.first())) {
        qCCritical(lcCardDAVAddressBookFeeder)
                << "Version" << versionStr
                << "is not supported. Supported versions are:" << supportedVersions.join(", ");
        return;
    }

    const auto baseNumber = PhoneNumberUtil::canonicalNumber(m_config.baseNumber);
    auto stripBaseNumber = [&baseNumber](QString num) {
        if (num.startsWith(baseNumber)) {
            num = num.sliced(baseNumber.size());
        }
        return num;
    };

    // Process vcard
    std::istringstream stringStream(data.toStdString());
    TextReader reader(stringStream);
    auto cards = reader.parseCards();

    for (auto &card : cards) {
        auto &props = card.properties();

        QString remoteUid;
        QString name;
        QString org;
        QString email;
        QByteArray photoData;
        QList<Contact::PhoneNumber> phoneNumbers;

        for (auto &prop : props) {
            const auto propName = prop.getName();

            if (propName == "UID") {
                remoteUid = QString::fromStdString(prop.getValue());
            } else if (propName == "FN") {
                name = QString::fromStdString(prop.getValue());
            } else if (propName == "ORG") {
                org = QString::fromStdString(prop.getValue());
            } else if (propName == "EMAIL") {
                email = QString::fromStdString(prop.getValue());
            } else if (propName == "TEL") {
                bool subscriptable = false;
                auto propParams = prop.params();
                for (const auto &param : propParams) {
                    if (param.first == "TYPE") {
                        const auto types = QString::fromStdString(param.second)
                                                   .split(QChar(','), Qt::SkipEmptyParts);
                        for (const auto &type : types) {
                            if (m_sipStatusSubscriptableAttributes.contains(
                                        type.trimmed().toLower())) {
                                subscriptable = true;
                                break;
                            }
                        }
                        if (subscriptable) {
                            break;
                        }
                    }
                }

                auto phoneNumber = QString::fromStdString(prop.getValue());
                phoneNumber = PhoneNumberUtil::isSipUri(phoneNumber)
                        ? phoneNumber
                        : PhoneNumberUtil::canonicalNumber(phoneNumber);
                phoneNumber = stripBaseNumber(phoneNumber);

                phoneNumbers.append({ Contact::NumberType::Unknown, phoneNumber, subscriptable });

            } else if (propName == "PHOTO") {

                auto propParams = prop.params();
                const auto end = propParams.end();

                bool isBase64 = false;
                bool isJpeg = false;

                for (auto it = propParams.begin(); it != end; ++it) {
                    if (it->first == "ENCODING" && it->second == "b") {
                        isBase64 = true;
                    } else if (it->first == "TYPE" && it->second == "JPEG") {
                        isJpeg = true;
                    }
                }

                if (isBase64 && isJpeg) {
                    photoData = QByteArray::fromStdString(prop.getValue());
                }
            }
        }

        if (!uuid.isEmpty() && !name.isEmpty() && !phoneNumbers.isEmpty()) {
            Contact *contact = AddressBook::instance().addContact(
                    uuid, remoteUid, { m_priority, m_displayName, m_group }, name, org, email,
                    modifiedDate, phoneNumbers, m_blockInfo);

            // Replaced cache entry -> not part of addressbook any more
            if (Contact *prevContact = m_cachedContacts.value(uuid, nullptr);
                prevContact && prevContact != contact) {
                prevContact->deleteLater();
            }

            m_cachedContacts.insert(uuid, contact);
            m_ignoredIds.remove(uuid);

            processPhotoProperty(contact->id(), photoData, modifiedDate);

        } else if (!uuid.isEmpty()) {
            m_ignoredIds.insert(uuid, modifiedDate);

            if (Contact *outdatedContact = m_cachedContacts.take(uuid)) {
                outdatedContact->deleteLater();
            }
        }
    }

    scheduleCacheWrite();
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
        qCCritical(lcCardDAVAddressBookFeeder) << "Unable to open file" << filePath;
        return;
    }

    QDataStream in(&cacheFile);

    quint16 magic = 0;
    quint8 version = 0;

    in >> magic;
    in >> version;

    if (in.status() != QDataStream::Ok || magic != CARDDAV_MAGIC || version != CARDDAV_VERSION) {
        qCInfo(lcCardDAVAddressBookFeeder) << "CardDAV cache file at" << filePath
                                           << "is invalid and will therefore be removed.";
        cacheFile.close();
        cacheFile.remove();
        return;
    }

    qsizetype numberOfContacts = 0;
    in >> m_syncTokens;
    in >> m_ignoredIds;
    in >> numberOfContacts;

    if (in.status() != QDataStream::Ok || numberOfContacts < 0 || numberOfContacts > 1000000) {
        qCWarning(lcCardDAVAddressBookFeeder) << "CardDAV cache file at" << filePath
                                              << "is corrupted and will therefore be removed.";
        m_syncTokens.clear();
        m_ignoredIds.clear();
        cacheFile.close();
        cacheFile.remove();
        return;
    }

    for (qsizetype i = 0; i < numberOfContacts; ++i) {
        QString key;
        Contact *contact = new Contact(this);
        in >> key;
        in >> *contact;

        if (in.status() != QDataStream::Ok) {
            delete contact;
            qCWarning(lcCardDAVAddressBookFeeder)
                    << "CardDAV cache file at" << filePath
                    << "is truncated or corrupted - aborting cache load.";
            qDeleteAll(m_cachedContacts);
            m_syncTokens.clear();
            m_cachedContacts.clear();
            m_ignoredIds.clear();
            cacheFile.close();
            cacheFile.remove();
            return;
        }

        m_cachedContacts.insert(key, contact);
    }

    cacheFile.close();

    qCInfo(lcCardDAVAddressBookFeeder)
            << "Loaded" << m_cachedContacts.size() << "contacts and" << m_syncTokens.size()
            << "sync tokens from cache with hash" << hash;
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

void CardDAVAddressBookFeeder::scheduleCacheWrite()
{
    if (!m_cacheWriteTimer.isActive()) {
        m_cacheWriteTimer.start();
    }
}

void CardDAVAddressBookFeeder::processPhotoProperty(const QString &id, const QByteArray &data,
                                                    const QDateTime &modifiedDate) const
{
    // Convert base64 data to image
    if (data.size()) {
        const QByteArray decoded = QByteArray::fromBase64(data);
        AvatarManager::instance().addExternalImage(id, decoded, modifiedDate);
    }
}

void CardDAVAddressBookFeeder::flushCacheImpl()
{
    const auto filePath = cacheFilePath(m_settingsHash, true);
    QFile cacheFile(filePath);

    if (!cacheFile.open(QIODevice::WriteOnly)) {
        qCCritical(lcCardDAVAddressBookFeeder) << "Unable to open file" << filePath;
        return;
    }

    QDataStream out(&cacheFile);
    out << quint16(CARDDAV_MAGIC);
    out << quint8(CARDDAV_VERSION);
    out << m_syncTokens;
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

    Q_EMIT AddressBook::instance().contactsReady();
}

void CardDAVAddressBookFeeder::process()
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(m_group);

    QHash<QString, QString> settingsHash;
    const auto keys = settings.allKeys();
    for (const auto &key : keys) {
        settingsHash.insert(key, settings.value(key, "").toString());
    }
    m_settingsHash = qHash(settingsHash);

    const bool useSSL = settings.value("useSSL", false).toBool();

    m_blockInfo.isBlocking = settings.value("block", false).toBool();
    m_blockInfo.responseCode =
            settings.value("blockSipCode", GONNECT_DEFAULT_BLOCK_SIP_CODE).toUInt();

    m_displayName = settings.value("displayName", m_group).toString();
    bool ok = true;
    m_priority = settings.value("prio", 0).toUInt(&ok);
    if (!ok) {
        qCWarning(lcCardDAVAddressBookFeeder) << "Could not parse priority value for" << m_group;
        m_priority = 0;
    }

    const auto subScriptableAttributes =
            settings.value("sipStatusSubscriptableAttributes", "").toStringList();
    m_sipStatusSubscriptableAttributes =
            subScriptableAttributes.isEmpty() ? QStringList() : subScriptableAttributes;

    // Unpack list in single string
    if (m_sipStatusSubscriptableAttributes.size() == 1
        && m_sipStatusSubscriptableAttributes.at(0).contains(',')) {
        m_sipStatusSubscriptableAttributes =
                m_sipStatusSubscriptableAttributes.at(0).split(',', Qt::SkipEmptyParts);
    }

    qCDebug(lcCardDAVAddressBookFeeder) << "Read subscribable attributes from config for" << m_group
                                        << ":" << m_sipStatusSubscriptableAttributes;

    for (QString &attr : m_sipStatusSubscriptableAttributes) {
        attr = std::move(attr).toLower().trimmed();
    }

    uint multigetBatchSize = settings.value("multigetBatchSize", 100).toUInt();
    if (multigetBatchSize < 1) {
        qCWarning(lcCardDAVAddressBookFeeder)
                << "Invalid multigetBatchSize for" << m_group << "- resetting to default of 100";
        multigetBatchSize = 100;
    }

    m_config = { settings.value("baseNumber", "").toString(),
                 settings.value("host", "").toString(),
                 settings.value("path", "").toString(),
                 settings.value("user", "").toString(),
                 settings.value("port", useSSL ? 443 : 80).toInt(),
                 useSSL,
                 multigetBatchSize };

    init();
    run();

    settings.endGroup();
};

QUrl CardDAVAddressBookFeeder::networkCheckURL() const
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(m_group);
    const bool useSSL = settings.value("useSSL", false).toBool();
    const QString host = settings.value("host", "").toString();
    const QString path = settings.value("path", "").toString();
    const int port = settings.value("port", useSSL ? 443 : 80).toInt();
    settings.endGroup();

    if (host.isEmpty()) {
        return QUrl();
    }

    QUrl url;
    url.setScheme(useSSL ? "https" : "http");
    url.setHost(host);
    url.setPort(port);
    url.setPath(path);
    return url;
}

#undef CARDDAV_MAGIC
#undef CARDDAV_VERSION
