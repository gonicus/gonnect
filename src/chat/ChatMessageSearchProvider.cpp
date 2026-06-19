#include <QLoggingCategory>

#include "ChatMessageSearchProvider.h"
#include "ChatMessageSearchIndexer.h"

#include "ChatConnectorManager.h"
#include "ChatMessage.h"

Q_LOGGING_CATEGORY(lcChatMessageSearchProvider, "gonnect.chat.message.search.provider")

/*
    The DB should persist across Gonnect restarts - however, how will we deal with messages having
    been added/deleted/edited while Gonnect and thus the DB weren't active?

    INSERT OR IGNORE INTO at ChatMessageSearchIndexer::addMessage() is just a temp hack
*/

ChatMessageSearchProvider::ChatMessageSearchProvider(QObject *parent) : QObject{ parent }
{
    m_model = new ChatMessageSearchModel(this);
    if (!m_model) {
        return;
    }

    m_indexer = new ChatMessageSearchIndexer(this);
    if (!m_indexer || !m_indexer->isInitialized()) {
        return;
    }

    connect(&ChatConnectorManager::instance(), &ChatConnectorManager::chatConnectorsChanged, this,
            &ChatMessageSearchProvider::updateChatProviders);

    connect(this, &ChatMessageSearchProvider::searchPhraseChanged, this, [this]() {
        if (m_model->rowCount() > 0) {
            m_model->reset();
        }

        if (m_searchPhrase.size() >= 3) {
            auto results = m_indexer->search(m_searchPhrase);
            if (results.isEmpty()) {
                qCWarning(lcChatMessageSearchProvider)
                        << "Message search did not return any results:" << m_indexer->lastError();
            } else {
                m_model->addResults(results);
            }
        }
    });
}

void ChatMessageSearchProvider::updateChatProviders()
{
    qCWarning(lcChatMessageSearchProvider) << "Syncing chat providers for search";

    auto providers = ChatConnectorManager::instance().chatConnectors();

    QHash<QString, bool> activeIds;
    activeIds.reserve(providers.size());

    for (auto provider : std::as_const(providers)) {
        if (!provider) {
            continue;
        }

        QString providerUid = provider->id();
        activeIds.insert(providerUid, true);

        // New providers
        if (!m_chatProvidersByUid.contains(providerUid)) {
            m_chatProvidersByUid.emplace(providerUid, provider);

            loadChatProvider(provider);
        }
        // TODO: Can pointers of providers that continue to exist change?
    }

    // Stale providers
    for (auto it = m_chatProvidersByUid.begin(); it != m_chatProvidersByUid.end();) {
        if (!activeIds.contains(it.key())) {
            it = m_chatProvidersByUid.erase(it);
        } else {
            ++it;
        }
    }
}

void ChatMessageSearchProvider::loadChatProvider(IChatProvider *provider)
{
    if (!provider) {
        return;
    }

    QString providerUid = provider->id();

    m_chatProvidersByUid.emplace(providerUid, provider);

    auto it = m_chatProvidersByUid.find(providerUid);
    if (it == m_chatProvidersByUid.end()) {
        return;
    }

    ProviderConnection &conn = it.value();
    QObject *context = conn.context.get();
    if (!context) {
        return;
    }

    const auto roomCount = provider->chatRoomsCount();
    for (int i = 0; i < roomCount; i++) {
        loadChatRoom(provider->chatRoomByIndex(i));
    }

    // Connect to chat provider changes as long as the associated context lives
    connect(provider, &IChatProvider::chatRoomAdded, context,
            [this](qsizetype, IChatRoom *room, QString) { loadChatRoom(room); });
    connect(provider, &IChatProvider::chatRoomRemoved, context,
            [this](qsizetype, IChatRoom *room) { removeChatRoom(room); });

    // TODO: Do chatRoomJoined/chatRoomLeft also emit added/removed?
}

void ChatMessageSearchProvider::loadChatRoom(IChatRoom *room)
{
    if (!room) {
        return;
    }

    QString roomUid = room->id();

    m_chatRoomsByUid.emplace(roomUid, room);

    auto it = m_chatRoomsByUid.find(roomUid);
    if (it == m_chatRoomsByUid.end()) {
        return;
    }

    RoomConnection &conn = it.value();
    QObject *context = conn.context.get();
    if (!context) {
        return;
    }

    QList<ChatMessageSearchIndexer::Message> messages;

    auto chatMessages = room->chatMessages();
    for (auto &chatMessage : chatMessages) {
        if (chatMessage && chatMessage->content()) {
            if (const auto textContent =
                        qobject_cast<ChatMessageContentText *>(chatMessage->content())) {
                messages.append({ chatMessage->eventId(), roomUid, textContent->rawText() });
            }
        }
    }

    if (!messages.isEmpty()) {
        if (!m_indexer->addMessages(messages)) {
            qCWarning(lcChatMessageSearchProvider)
                    << "Failed to add messages to indexer:" << m_indexer->lastError();
        };
        m_indexer->optimize();
    }

    // Connect to chat room changes as long as the associated context lives
    connect(room, &IChatRoom::chatMessageAdded, context,
            [this, roomUid](qsizetype, ChatMessage *chatMessage) {
                if (chatMessage && chatMessage->content()) {
                    if (const auto textContent =
                                qobject_cast<ChatMessageContentText *>(chatMessage->content())) {
                        m_indexer->addMessage(
                                { chatMessage->eventId(), roomUid, textContent->rawText() });
                    }
                }
            });
    connect(room, &IChatRoom::chatMessageRemoved, context,
            [this](qsizetype, ChatMessage *chatMessage) {
                if (chatMessage) {
                    m_indexer->removeMessage(chatMessage->eventId());
                }
            });
    connect(room, &IChatRoom::chatMessageContentChanged, context,
            [this, roomUid](qsizetype, ChatMessage *chatMessage) {
                if (chatMessage && chatMessage->content()) {
                    if (const auto textContent =
                                qobject_cast<ChatMessageContentText *>(chatMessage->content())) {
                        m_indexer->updateMessage(
                                { chatMessage->eventId(), roomUid, textContent->rawText() });
                    }
                }
            });
    connect(room, &IChatRoom::chatMessagesReset, context,
            [this, roomUid]() { m_indexer->removeMessagesByRoom(roomUid); });
}

void ChatMessageSearchProvider::removeChatRoom(IChatRoom *room)
{
    if (!room) {
        return;
    }

    QString roomUid = room->id();

    m_chatRoomsByUid.remove(roomUid);

    m_indexer->removeMessagesByRoom(roomUid);
}

ChatMessage *ChatMessageSearchProvider::getChatMessage(const QString &roomUid,
                                                       const QString &messageUid)
{
    auto it = m_chatRoomsByUid.find(roomUid);
    if (it == m_chatRoomsByUid.end()) {
        return nullptr;
    }

    RoomConnection &conn = it.value();
    auto room = conn.room;
    if (!room) {
        return nullptr;
    }

    return room->chatMessageById(messageUid);
}

QString ChatMessageSearchProvider::getChatMessageText(ChatMessage *message)
{
    if (!message) {
        return "";
    }

    if (const auto textContent = qobject_cast<ChatMessageContentText *>(message->content())) {
        return textContent->rawText();
    }

    return "";
}

ChatMessageSearchProvider::~ChatMessageSearchProvider()
{
    if (m_model) {
        delete m_model;
        m_model = nullptr;
    }

    m_chatProvidersByUid.clear();
    m_chatRoomsByUid.clear();
}
