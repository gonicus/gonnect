#pragma once

#include <QObject>
#include <QDateTime>

#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

struct sqlite3;
struct sqlite3_stmt;

class ChatMessageSearchIndexer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ChatMessageSearchIndexer)

public:
    Q_REQUIRED_RESULT static ChatMessageSearchIndexer &instance()
    {
        static ChatMessageSearchIndexer *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new ChatMessageSearchIndexer();
        }

        return *_instance;
    }

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
        QString id;
        QString source;
        QString body;
    };

    struct SearchResult
    {
        QString id;
        double rank; // FTS5 BM25 rank: negative == better
    };

    bool addMessage(const Message &message);
    bool addMessages(const QList<Message> &messages);

    bool removeMessage(const QString &id);
    bool removeMessagesBySource(const QString &source);

    bool updateMessage(const Message &message);

    QList<SearchResult> search(const QString &query, int limit = 20);

    bool optimize();

private:
    ChatMessageSearchIndexer(QObject *parent = nullptr);

    bool exec(const QString &statement);

    sqlite3 *m_db = nullptr;
    QString m_error;
};

class ChatMessageSearchIndexerWrapper
{
    Q_GADGET
    QML_FOREIGN(ChatMessageSearchIndexer)
    QML_NAMED_ELEMENT(ChatMessageSearchIndexer)
    QML_SINGLETON

public:
    static ChatMessageSearchIndexer *create(QQmlEngine *, QJSEngine *)
    {
        return &ChatMessageSearchIndexer::instance();
    }

private:
    ChatMessageSearchIndexerWrapper() = default;
};
