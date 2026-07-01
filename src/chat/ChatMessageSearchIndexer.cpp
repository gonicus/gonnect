#include <QLoggingCategory>
#include <QRegularExpression>

#include "ReadOnlyConfdSettings.h"
#include "ChatMessageSearchIndexer.h"

// INFO: Uses SQLCipher amalgamation (AES-256 + FTS5): https://github.com/sqlcipher/sqlcipher
#include <sqlite3.h>

Q_LOGGING_CATEGORY(lcChatMessageSearchIndexer, "gonnect.chat.message.search.indexer")

ChatMessageSearchIndexer::ChatMessageSearchIndexer(QObject *parent) : QObject{ parent }
{
    m_preprocessor = new ChatMessageSearchPreprocessor(this);
    if (!m_preprocessor || !m_preprocessor->isInitialized()) {
        return;
    }

    // TODO: Use proper settings names, wallet, etc.
    ReadOnlyConfdSettings settings;
    const QByteArray path = settings.value("generic/chatSearchCachePath", "").toString().toUtf8();
    const QByteArray password =
            settings.value("generic/chatSearchCachePassword", "").toString().toUtf8();

    int status = sqlite3_open(path, &m_db);
    if (status != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));

        sqlite3_close(m_db);
        m_db = nullptr;

        return;
    }

    status = sqlite3_key(m_db, password, password.size());
    if (status != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));

        sqlite3_close(m_db);
        m_db = nullptr;

        return;
    }

    QString initStatement = tr(
            // Performance pragmas (safe with WAL)
            "PRAGMA journal_mode = WAL;"
            "PRAGMA synchronous = NORMAL;"

            // Mapping table
            "CREATE TABLE IF NOT EXISTS messages_map ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    message_uid TEXT UNIQUE,"
            "    room_uid TEXT,"
            "    timestamp INTEGER"
            ");"

            // FTS5 virtual table
            // requires SQLite >= 3.43.0 for contentless_delete (by id only)
            // requires SQLite >= 3.38.0 for the trigram tokenizer
            // INFO: Using messages_fts.rowid == messages_map.id
            "CREATE VIRTUAL TABLE IF NOT EXISTS messages_fts USING fts5("
            "    body,"
            "    content='',"
            "    contentless_delete=1,"
            "    tokenize='trigram'"
            ");"

            "CREATE INDEX IF NOT EXISTS idx_messages_map ON messages_map(room_uid);");
    if (!exec(initStatement)) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));

        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    m_isInitialized = true;
}

ChatMessageSearchIndexer::~ChatMessageSearchIndexer()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }

    if (m_preprocessor) {
        delete m_preprocessor;
        m_preprocessor = nullptr;
    }
}

bool ChatMessageSearchIndexer::addMessage(const Message &message)
{
    if (!m_db) {
        return false;
    }

    // Mapping
    const QString mappingStatement =
            "INSERT OR IGNORE INTO messages_map(message_uid, room_uid, timestamp) VALUES (?,?,?);";
    const QByteArray message_uid = message.messageUid.toUtf8();
    const QByteArray room_uid = message.roomUid.toUtf8();

    Statement mapping;
    if (sqlite3_prepare_v2(m_db, mappingStatement.toUtf8(), -1, &mapping.statement, nullptr)
        != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_text(mapping.statement, 1, message_uid.constData(), -1, SQLITE_STATIC);
    sqlite3_bind_text(mapping.statement, 2, room_uid.constData(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(mapping.statement, 3, message.timestamp);

    if (sqlite3_step(mapping.statement) != SQLITE_DONE) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }

    // FTS
    const QString ftsStatement = "INSERT OR IGNORE INTO messages_fts(rowid, body) VALUES (?,?);";
    const sqlite_int64 inserted_id = sqlite3_last_insert_rowid(m_db);
    const QByteArray body = m_preprocessor->process(message.body).toUtf8();

    Statement fts;
    if (sqlite3_prepare_v2(m_db, ftsStatement.toUtf8(), -1, &fts.statement, nullptr) != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_int64(fts.statement, 1, inserted_id);
    sqlite3_bind_text(fts.statement, 2, body.constData(), -1, SQLITE_STATIC);

    if (sqlite3_step(fts.statement) != SQLITE_DONE) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }

    return true;
}

bool ChatMessageSearchIndexer::addMessages(const QList<Message> &messages)
{
    if (!m_db) {
        return false;
    }

    if (messages.isEmpty()) {
        return true;
    }

    if (!exec("BEGIN TRANSACTION;")) {
        return false;
    }

    for (const Message &message : messages) {
        if (!addMessage(message)) {
            exec("ROLLBACK;");
            return false;
        }
    }

    return exec("COMMIT;");
}

bool ChatMessageSearchIndexer::removeMessage(const QString &id)
{
    if (!m_db) {
        return false;
    }

    // Mapping
    const QString mappingStatement = "DELETE FROM messages_map WHERE message_uid = ? RETURNING id;";

    Statement mapping;
    if (sqlite3_prepare_v2(m_db, mappingStatement.toUtf8(), -1, &mapping.statement, nullptr)
        != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_text(mapping.statement, 1, id.toUtf8(), -1, SQLITE_STATIC);

    int deleted_id = -1;
    if (sqlite3_step(mapping.statement) == SQLITE_ROW) {
        deleted_id = sqlite3_column_int(mapping.statement, 0);
    } else {
        return false;
    }

    // FTS
    const QString ftsStatement = "DELETE FROM messages_fts WHERE rowid = ?;";

    Statement fts;
    if (sqlite3_prepare_v2(m_db, ftsStatement.toUtf8(), -1, &fts.statement, nullptr) != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_int64(fts.statement, 1, deleted_id);

    if (sqlite3_step(fts.statement) != SQLITE_DONE) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }

    return true;
}

bool ChatMessageSearchIndexer::removeMessagesByRoom(const QString &roomUid)
{
    if (!m_db) {
        return false;
    }

    // FTS
    const QString ftsStatement = "DELETE FROM messages_fts WHERE rowid IN (SELECT id FROM "
                                 "messages_map WHERE room_uid = ?)";

    Statement fts;
    if (sqlite3_prepare_v2(m_db, ftsStatement.toUtf8(), -1, &fts.statement, nullptr) != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_text(fts.statement, 1, roomUid.toUtf8(), -1, SQLITE_STATIC);

    if (sqlite3_step(fts.statement) != SQLITE_DONE) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }

    // Mapping
    const QString mappingStatement = "DELETE FROM messages_map WHERE room_uid = ?;";

    Statement mapping;
    if (sqlite3_prepare_v2(m_db, mappingStatement.toUtf8(), -1, &mapping.statement, nullptr)
        != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_text(mapping.statement, 1, roomUid.toUtf8(), -1, SQLITE_STATIC);

    if (sqlite3_step(mapping.statement) != SQLITE_DONE) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }

    return true;
}

bool ChatMessageSearchIndexer::removeMessagesByStaleDate(qint64 timestamp)
{
    if (!m_db) {
        return false;
    }

    // FTS
    const QString ftsStatement = "DELETE FROM messages_fts WHERE rowid IN (SELECT id FROM "
                                 "messages_map WHERE timestamp < ?)";

    Statement fts;
    if (sqlite3_prepare_v2(m_db, ftsStatement.toUtf8(), -1, &fts.statement, nullptr) != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_int64(fts.statement, 1, timestamp);

    if (sqlite3_step(fts.statement) != SQLITE_DONE) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }

    // Mapping
    const QString mappingStatement = "DELETE FROM messages_map WHERE timestamp < ?;";

    Statement mapping;
    if (sqlite3_prepare_v2(m_db, mappingStatement.toUtf8(), -1, &mapping.statement, nullptr)
        != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }
    sqlite3_bind_int64(mapping.statement, 1, timestamp);

    if (sqlite3_step(mapping.statement) != SQLITE_DONE) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return false;
    }

    return true;
}

bool ChatMessageSearchIndexer::updateMessage(const Message &message)
{
    if (!exec("BEGIN TRANSACTION;")) {
        return false;
    }

    if (!removeMessage(message.messageUid) || !addMessage(message)) {
        exec("ROLLBACK;");
        return false;
    }

    return exec("COMMIT;");
}

QString ChatMessageSearchIndexer::getLatestMessageUid()
{
    if (!m_db) {
        return "";
    }

    const QString queryStatement =
            "SELECT message_uid FROM messages_map ORDER BY timestamp DESC LIMIT 1;";

    Statement query;
    if (sqlite3_prepare_v2(m_db, queryStatement.toUtf8(), -1, &query.statement, nullptr)
        != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return "";
    }

    if (sqlite3_step(query.statement) == SQLITE_ROW) {
        const auto *latestMessageUid = sqlite3_column_text(query.statement, 0);
        if (latestMessageUid) {
            return QString::fromUtf8(reinterpret_cast<const char *>(latestMessageUid));
        }
    }

    return "";
}

QList<ChatMessageSearchIndexer::SearchResult> ChatMessageSearchIndexer::search(const QString &query,
                                                                               int limit)
{
    QList<SearchResult> results;

    if (!m_db) {
        return results;
    }

    // INFO: The FTS5 trigram tokenizer needs at least 3 characters to produce a
    // token. Shorter queries would match nothing (or cause an FTS error).
    QString processedQuery = m_preprocessor->process(query.simplified());
    if (processedQuery.length() < 3) {
        return results;
    }

    // Strip FTS5 operator characters that would cause query parse errors.
    static const QRegularExpression ftsSpecial(QStringLiteral(R"([\"*()\^:])"));
    QString safeQuery = processedQuery.remove(ftsSpecial).simplified();
    if (safeQuery.isEmpty()) {
        return results;
    }

    // FTS5 rank is a special hidden column; expose it explicitly (unqualified in ORDER BY).
    // Results are ordered ascending by rank (FTS5 BM25 is negative: closer
    // to zero = worse; more negative = better match).
    const QString searchStatement =
            "SELECT m.message_uid, m.room_uid, f.rank FROM messages_fts f JOIN "
            "messages_map m ON f.rowid = m.id "
            "WHERE messages_fts MATCH ? ORDER BY rank LIMIT ?;";

    Statement search;
    if (sqlite3_prepare_v2(m_db, searchStatement.toUtf8(), -1, &search.statement, nullptr)
        != SQLITE_OK) {
        m_error = QString::fromUtf8(sqlite3_errmsg(m_db));
        return results;
    }

    const QByteArray queryBytes = safeQuery.toUtf8();
    sqlite3_bind_text(search.statement, 1, queryBytes.constData(), -1, SQLITE_STATIC);
    sqlite3_bind_int(search.statement, 2, limit);

    while (sqlite3_step(search.statement) == SQLITE_ROW) {
        SearchResult result;

        const auto *messageUid = sqlite3_column_text(search.statement, 0);
        if (messageUid) {
            result.messageUid = QString::fromUtf8(reinterpret_cast<const char *>(messageUid));
        }

        const auto *roomUid = sqlite3_column_text(search.statement, 1);
        if (roomUid) {
            result.roomUid = QString::fromUtf8(reinterpret_cast<const char *>(roomUid));
        }

        result.rank = sqlite3_column_double(search.statement, 2);
        results.append(result);
    }

    return results;
}

bool ChatMessageSearchIndexer::optimize()
{
    if (m_db) {
        // FTS5 optimize merges all segment B-trees into one for faster reads.
        return exec("INSERT INTO messages_fts(messages_fts) VALUES('optimize');");
    } else {
        return false;
    }
}

bool ChatMessageSearchIndexer::exec(const QString &statement)
{
    char *error = nullptr;

    int status = sqlite3_exec(m_db, statement.toUtf8(), nullptr, nullptr, &error);
    if (status != SQLITE_OK) {
        m_error = QString::fromUtf8(error);
        sqlite3_free(error);

        return false;
    }

    return true;
}

void ChatMessageSearchIndexer::Statement::finalize(sqlite3_stmt *statement)
{
    // Wraps sqlite3_finalize() in order to avoid sqlite3.h inclusion on this header
    sqlite3_finalize(statement);
}
