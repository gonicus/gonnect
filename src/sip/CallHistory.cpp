#include "CallHistory.h"
#include "ErrorBus.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(lcCallHistory, "gonnect.app.CallHistory")

CallHistory::CallHistory(QObject *parent) : QObject{ parent }
{

    m_databasePath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/callhistory.db";
    QFileInfo dbPathInfo(m_databasePath);

    if (!dbPathInfo.exists()) {
        const bool success = dbPathInfo.dir().mkpath(
                QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        if (!success) {
            qCCritical(lcCallHistory)
                    << "Unable to create directory"
                    << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                    << "for call history database";
            ErrorBus::instance().addFatalError(
                    tr("Failed to create directory %1 to store the call history database.")
                            .arg(QStandardPaths::writableLocation(
                                    QStandardPaths::AppDataLocation)));
            return;
        }
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(m_databasePath);

    ensureDatabaseVersion();
    readFromDatabase();
}

qsizetype CallHistory::insertItemAtCorrectPosition(CallHistoryItem *item)
{
    Q_CHECK_PTR(item);
    Q_ASSERT(!m_historyItems.contains(item));

    for (qsizetype i = 0; i < m_historyItems.size(); ++i) {
        const auto currItem = m_historyItems.at(i);
        if (item->time() > currItem->time()) {
            m_historyItems.insert(i, item);
            writeToDatabase(*item);
            return i;
        }
    }

    m_historyItems.push_back(item);
    writeToDatabase(*item);

    return m_historyItems.size() - 1;
}

void CallHistory::ensureDatabaseVersion()
{

    const QFileInfo dbScriptsDirInfo(":/migrate/scripts/callhistory/");
    const QFileInfo fileInfo(m_databasePath);
    const bool dbExisted = fileInfo.exists();

    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcCallHistory) << "Unable to open call history database:"
                                  << db.lastError().text();
        ErrorBus::instance().addFatalError(
                tr("Failed to open call history database: %1").arg(db.lastError().text()));
        return;
    }
    qCInfo(lcCallHistory) << "Successfully opened history database";

    const auto versionOfFileName = [](const QString &fileName) -> QString {
        return fileName.split(QChar('.')).at(0);
    };

    const auto scriptInfos = dbScriptsDirInfo.dir().entryInfoList(
            { "*.sql" }, QDir::Files | QDir::Readable, QDir::Name);
    const auto latestVersionNumber = versionOfFileName(scriptInfos.last().fileName());

    // Determine current version in the database
    QString currentVersionNumber = "000";
    if (dbExisted) {
        // Database file exist but version table does not: bail out and ask user to remove the
        // database
        if (!db.tables().contains("appinfo")) {
            qCCritical(lcCallHistory)
                    << "Database does exist but 'appinfo' table does not. This is an inconsistent "
                       "state. Please remove"
                    << m_databasePath
                    << "and restart the app. The database will be re-initialized then.";
            ErrorBus::instance().addFatalError(
                    tr("Call history database is inconsistent. Please remove %1 and restart the "
                       "App to re-initialize the database.")
                            .arg(m_databasePath));
            return;
        }

        QSqlQuery query(db);
        if (query.prepare("SELECT value FROM appinfo WHERE key = 'db_scheme_version';")
            && query.exec()) {
            if (query.next()) {
                currentVersionNumber = query.value(0).toString();
            } else {
                qCCritical(lcCallHistory) << "Querying the scheme version in the database returned "
                                             "an empty result set";
                return;
            }

        } else {
            qCCritical(lcCallHistory)
                    << "Querying the scheme version in the database yields an error:"
                    << query.lastError().text();
            return;
        }
    }

    // Update scheme to latest version
    if (currentVersionNumber < latestVersionNumber) {
        qCInfo(lcCallHistory) << "Migrating database scheme from" << currentVersionNumber << "to"
                              << latestVersionNumber;

        for (const auto &scriptInfo : scriptInfos) {
            const auto version = versionOfFileName(scriptInfo.fileName());
            if (currentVersionNumber >= version) {
                continue;
            }

            QFile file(scriptInfo.filePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qCCritical(lcCallHistory) << "Unable to open databse scheme file" << scriptInfo
                                          << ":" << file.errorString();
                return;
            }

            qCInfo(lcCallHistory) << "Executing sql from file" << scriptInfo.filePath();

            QTextStream in(&file);
            const auto queries = in.readAll().split(QChar(';'));

            for (const auto &queryString : queries) {
                if (!queryString.trimmed().isEmpty()) {
                    QSqlQuery query(db);
                    if (!query.prepare(queryString) || !query.exec()) {
                        qCCritical(lcCallHistory)
                                << "Error on executing SQL query:" << query.lastError().text();
                    }
                }
            }
        }

        // Write latest version into appinfo table
        QSqlQuery query(db);
        query.prepare("UPDATE appinfo SET value = :dbVersion WHERE key = 'db_scheme_version';");
        query.bindValue(":dbVersion", latestVersionNumber);

        if (!query.exec()) {
            qCCritical(lcCallHistory)
                    << "Unable to write scheme version into database:" << query.lastError().text();
            return;
        } else {
            qCInfo(lcCallHistory) << "Updated database scheme to version" << latestVersionNumber;
        }
    } else {
        qCInfo(lcCallHistory) << "Database scheme is up to date at" << currentVersionNumber;
    }
}

void CallHistory::writeToDatabase(CallHistoryItem &item)
{
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcCallHistory) << "Unable to open call history database:"
                                  << db.lastError().text();
    } else {
        qCInfo(lcCallHistory) << "Successfully opened history database";

        QSqlQuery query(db);

        if (item.dataBaseId() < 0) {
            qCInfo(lcCallHistory) << "Writing new history item to database";
            query.prepare("INSERT INTO history (time, remoteUrl, account, type, durationSeconds, "
                          "contactId, isSipSubscriptable) VALUES (:time, :remoteUrl, :account, "
                          ":type, :durationSeconds, :contactId, :isSipSubscriptable);");
        } else {
            qCInfo(lcCallHistory) << "Updating new history item with id" << item.dataBaseId()
                                  << "in database";
            query.prepare("UPDATE history SET time = :time, remoteUrl = :remoteUrl, account = "
                          ":account, type = :type, durationSeconds = :durationSeconds, contactId = "
                          ":contactId, isSipSubscriptable = :isSipSubscriptable WHERE id = :id;");
            query.bindValue(":id", item.dataBaseId());
        }

        query.bindValue(":time", item.time().toSecsSinceEpoch());
        query.bindValue(":remoteUrl", item.remoteUrl());
        query.bindValue(":account", item.account());
        query.bindValue(":type", static_cast<int>(item.type()));
        query.bindValue(":durationSeconds", item.durationSeconds());
        query.bindValue(":contactId", item.contactId());
        query.bindValue(":isSipSubscriptable", item.isSipSubscriptable());

        if (!query.exec()) {
            qCCritical(lcCallHistory)
                    << "Error on executing SQL query:" << query.lastError().text();
        } else if (item.dataBaseId() < 0) {
            item.setDataBaseId(query.lastInsertId().toLongLong());
        }
    }
}

void CallHistory::readFromDatabase()
{
    m_historyItems.clear();

    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcCallHistory) << "Unable to open call history database:"
                                  << db.lastError().text();
    } else {
        qCInfo(lcCallHistory) << "Successfully opened history database";
        qCInfo(lcCallHistory) << "Collecting history from database";

        QSqlQuery query(db);
        query.prepare("SELECT * FROM history ORDER BY time DESC;");

        if (!query.exec()) {
            qCCritical(lcCallHistory)
                    << "Error on executing SQL query:" << query.lastError().text();
        } else {
            while (query.next()) {
                auto type = static_cast<CallHistoryItem::Types>(query.value("type").toInt());

                // SIPCall flag was introduced later, so set it for legacy call history items
                if (!(type & CallHistoryItem::Type::SIPCall)
                    && !(type & CallHistoryItem::Type::JitsiMeetCall)) {
                    type |= CallHistoryItem::Type::SIPCall;
                }

                auto item = new CallHistoryItem(
                        QDateTime::fromSecsSinceEpoch(query.value("time").toLongLong()),
                        query.value("remoteUrl").toString(), query.value("account").toString(),
                        query.value("contactId").toString(),
                        query.value("isSipSubscriptable").toBool(), query.value("id").toLongLong(),
                        query.value("durationSeconds").toUInt(), type, this);

                m_historyItems.push_back(item);
            }
        }

        qCInfo(lcCallHistory) << "Read" << m_historyItems.size() << "history items from database";
    }
}

ContactInfo CallHistory::lastOutgoingSipInfo() const
{
    for (const auto historyItem : std::as_const(m_historyItems)) {
        if (historyItem->type() == CallHistoryItem::Type::Outgoing) {
            const auto callInfo =
                    PhoneNumberUtil::instance().contactInfoBySipUrl(historyItem->remoteUrl());
            if (!callInfo.isAnonymous) {
                return callInfo;
            }
        }
    }
    return ContactInfo();
}

CallHistoryItem *CallHistory::addHistoryItem(CallHistoryItem *item)
{
    Q_CHECK_PTR(item);
    if (!m_historyItems.contains(item)) {
        item->setParent(this);
        const auto index = insertItemAtCorrectPosition(item);
        emit itemAdded(index, item);
    }
    return item;
}

CallHistoryItem *CallHistory::addHistoryItem(CallHistoryItem::Types type, const QString &account,
                                             const QString &remoteUrl, const QString &contactId,
                                             bool isSipSubscriptable)
{

    auto item = new CallHistoryItem(remoteUrl, account, contactId, isSipSubscriptable, type, this);
    const auto index = insertItemAtCorrectPosition(item);
    emit itemAdded(index, item);
    return item;
}

qsizetype CallHistory::indexOfItem(const CallHistoryItem *item) const
{
    return m_historyItems.indexOf(item);
}

QDebug operator<<(QDebug debug, const CallHistory &history)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "CallHistory (" << history.historyItems().size() << " items)";
    return debug;
}
