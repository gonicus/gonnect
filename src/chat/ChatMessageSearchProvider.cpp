#include "ChatMessageSearchProvider.h"
#include "ChatConnectorManager.h"
#include "ChatMessage.h"

#include "ChatMessageSearchIndexer.h"

ChatMessageSearchProvider::ChatMessageSearchProvider(QObject *parent) : QObject{ parent }
{
    m_model = new ChatMessageSearchModel(this);

    connect(&ChatConnectorManager::instance(), &ChatConnectorManager::chatConnectorsChanged, this,
            &ChatMessageSearchProvider::resetChatProviders);

    connect(this, &ChatMessageSearchProvider::chatRoomAdded, this, [this](QString uid) {
        auto room = m_chatRoomsByUid[uid];
        auto context = m_chatRoomContextsByUid[uid];
        if (room && context) {
            QList<ChatMessageSearchIndexer::Message> messages;

            auto chatMessages = room->chatMessages();
            for (auto &chatMessage : chatMessages) {
                if (chatMessage && chatMessage->content()) {
                    if (const auto textContent =
                                qobject_cast<ChatMessageContentText *>(chatMessage->content())) {
                        messages.append({ chatMessage->eventId(), uid, textContent->rawText() });
                    }
                }
            }

            ChatMessageSearchIndexer::instance().addMessages(messages);
            ChatMessageSearchIndexer::instance().optimize();

            // Connect to chat room changes as long as the associated context lives
            connect(room, &IChatRoom::chatMessageAdded, context,
                    [uid](qsizetype, ChatMessage *chatMessage) {
                        if (chatMessage && chatMessage->content()) {
                            if (const auto textContent = qobject_cast<ChatMessageContentText *>(
                                        chatMessage->content())) {
                                ChatMessageSearchIndexer::instance().addMessage(
                                        { chatMessage->eventId(), uid, textContent->rawText() });
                            }
                        }
                    });
            connect(room, &IChatRoom::chatMessageRemoved, context,
                    [](qsizetype, ChatMessage *chatMessage) {
                        if (chatMessage) {
                            ChatMessageSearchIndexer::instance().removeMessage(
                                    chatMessage->eventId());
                        }
                    });
            connect(room, &IChatRoom::chatMessageContentChanged, context,
                    [uid](qsizetype, ChatMessage *chatMessage) {
                        if (chatMessage && chatMessage->content()) {
                            if (const auto textContent = qobject_cast<ChatMessageContentText *>(
                                        chatMessage->content())) {
                                ChatMessageSearchIndexer::instance().updateMessage(
                                        { chatMessage->eventId(), uid, textContent->rawText() });
                            }
                        }
                    });
            connect(room, &IChatRoom::chatMessagesReset, context,
                    [uid]() { ChatMessageSearchIndexer::instance().removeMessagesBySource(uid); });
        }
    });

    connect(this, &ChatMessageSearchProvider::chatRoomDeleted, this,
            [](QString uid) { ChatMessageSearchIndexer::instance().removeMessagesBySource(uid); });

    connect(this, &ChatMessageSearchProvider::searchPhraseChanged, this, [this]() {
        m_model->reset();

        auto results = ChatMessageSearchIndexer::instance().search(m_searchPhrase);
        if (!results.isEmpty()) {
            m_model->addResults(results);
        }
    });

    resetChatProviders();
}

void ChatMessageSearchProvider::resetChatProviders()
{
    m_chatRoomsByUid.clear();
    qDeleteAll(m_chatRoomContextsByUid);
    m_chatRoomContextsByUid.clear();

    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }

    m_chatProviders = ChatConnectorManager::instance().chatConnectors();
    if (!m_chatProviders.isEmpty()) {
        m_chatProviderContext = new QObject(this);

        for (auto provider : std::as_const(m_chatProviders)) {
            if (!provider) {
                continue;
            }

            const auto roomCount = provider->chatRoomsCount();
            for (int i = 0; i < roomCount; i++) {
                auto room = provider->chatRoomByIndex(i);
                if (room) {
                    QString uid = QString("%1-%2").arg(room->id(), provider->id());

                    m_chatRoomsByUid.insert(uid, room);
                    m_chatRoomContextsByUid.insert(uid, new QObject(this));

                    Q_EMIT chatRoomAdded(uid);
                }
            }

            // Connect to chat provider changes as long as the shared context lives
            connect(provider, &IChatProvider::chatRoomAdded, m_chatProviderContext,
                    [this, provider](qsizetype, IChatRoom *room, QString) {
                        if (room) {
                            QString uid = QString("%1-%2").arg(room->id(), provider->id());

                            m_chatRoomsByUid.insert(uid, room);
                            m_chatRoomContextsByUid.insert(uid, new QObject(this));

                            Q_EMIT chatRoomAdded(uid);
                        }
                    });
            connect(provider, &IChatProvider::chatRoomRemoved, m_chatProviderContext,
                    [this, provider](qsizetype, IChatRoom *room) {
                        if (room) {
                            QString uid = QString("%1-%2").arg(room->id(), provider->id());

                            if (m_chatRoomsByUid.remove(uid)) {
                                m_chatRoomContextsByUid[uid]->deleteLater();
                                m_chatRoomContextsByUid[uid] = nullptr;

                                Q_EMIT chatRoomDeleted(uid);
                            }
                        }
                    });
        }
    }
}

ChatMessageSearchProvider::~ChatMessageSearchProvider()
{
    if (m_model) {
        delete m_model;
        m_model = nullptr;
    }

    m_chatRoomsByUid.clear();
    qDeleteAll(m_chatRoomContextsByUid);
    m_chatRoomContextsByUid.clear();

    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }
}
