#pragma once

#include <memory>

#include <QObject>
#include <QQmlEngine>

#include "IChatRoom.h"

#include "ChatMessageSearchIndexer.h"
#include "ChatMessageSearchModel.h"

class IChatProvider;

class ChatMessageSearchProvider : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ChatMessageSearchProvider)

    Q_PROPERTY(ChatMessageSearchModel *model READ model CONSTANT)
    Q_PROPERTY(QString searchPhrase MEMBER m_searchPhrase NOTIFY searchPhraseChanged)

public:
    Q_REQUIRED_RESULT static ChatMessageSearchProvider &instance()
    {
        static ChatMessageSearchProvider *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new ChatMessageSearchProvider();
        }

        return *_instance;
    }

    ~ChatMessageSearchProvider();

    ChatMessageSearchModel *model() const { return m_model; }

    void updateChatProviders();
    void loadChatProvider(IChatProvider *provider);

    void loadChatRoom(IChatRoom *room);
    void removeChatRoom(IChatRoom *room);

    Q_INVOKABLE ChatMessage *getChatMessage(const QString &roomUid, const QString &messageUid);
    Q_INVOKABLE QString getChatMessageText(ChatMessage *message);

private:
    ChatMessageSearchProvider(QObject *parent = nullptr);

    ChatMessageSearchIndexer *m_indexer = nullptr;
    ChatMessageSearchModel *m_model = nullptr;

    QString m_searchPhrase;

    struct ProviderConnection
    {
        std::shared_ptr<QObject> context;
        IChatProvider *provider = nullptr;

        ProviderConnection() : context(std::make_shared<QObject>()) { }

        ProviderConnection(IChatProvider *provider)
            : context(std::make_shared<QObject>()), provider(provider)
        {
        }
    };

    struct RoomConnection
    {
        std::shared_ptr<QObject> context;
        IChatRoom *room = nullptr;

        RoomConnection() : context(std::make_shared<QObject>()) { }

        RoomConnection(IChatRoom *room) : context(std::make_shared<QObject>()), room(room) { }
    };

    QHash<QString, ProviderConnection> m_chatProvidersByUid;
    QHash<QString, RoomConnection> m_chatRoomsByUid;

Q_SIGNALS:
    void chatRoomAdded(QString uid);
    void chatRoomDeleted(QString uid);

    void chatMessagesChanged();

    void searchPhraseChanged();
};

class ChatMessageSearchProviderWrapper
{
    Q_GADGET
    QML_FOREIGN(ChatMessageSearchProvider)
    QML_NAMED_ELEMENT(ChatMessageSearchProvider)
    QML_SINGLETON

public:
    static ChatMessageSearchProvider *create(QQmlEngine *, QJSEngine *)
    {
        return &ChatMessageSearchProvider::instance();
    }

private:
    ChatMessageSearchProviderWrapper() = default;
};
