#pragma once

#include <QObject>
#include <QDateTime>

#include "ChatMessageSearchPreprocessor.h"

struct sqlite3;
struct sqlite3_stmt;

class ChatMessageSearchIndexer : public QObject
{
    Q_OBJECT

public:
    ChatMessageSearchIndexer(QObject *parent = nullptr);
    ~ChatMessageSearchIndexer();

    bool isOpen() const { return m_db != nullptr; }
    QString lastError() const { return m_error; }

    struct Statement
    {
        sqlite3_stmt *statement = nullptr;

        void finalize(sqlite3_stmt *statement);

        ~Statement()
        {
            if (statement) {
                finalize(statement);
            }
        }
    };

    struct Message
    {
        QString messageUid;
        QString roomUid;
        QString body;
    };

    struct SearchResult
    {
        QString messageUid;
        double rank = 0.0; // FTS5 BM25 rank: negative == better
    };

    bool addMessage(const Message &message);
    bool addMessages(const QList<Message> &messages);

    bool removeMessage(const QString &id);
    bool removeMessagesByRoom(const QString &roomUid);

    bool updateMessage(const Message &message);

    QList<SearchResult> search(const QString &query, int limit = 20);

    bool optimize();

private:
    bool exec(const QString &statement);

    ChatMessageSearchPreprocessor *m_preprocessor = nullptr;

    sqlite3 *m_db = nullptr;
    QString m_error;
};
