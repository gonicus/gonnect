#include <QLoggingCategory>

#include "ChatMessageSearchProvider.h"
#include "ChatMessageSearchIndexer.h"

#include "ChatConnectorManager.h"
#include "ChatMessage.h"

Q_LOGGING_CATEGORY(lcChatMessageSearchProvider, "gonnect.chat.message.search.provider")

ChatMessageSearchProvider::ChatMessageSearchProvider(QObject *parent) : QObject{ parent }
{
    m_model = new ChatMessageSearchModel(this);

    connect(&ChatConnectorManager::instance(), &ChatConnectorManager::chatConnectorsChanged, this,
            &ChatMessageSearchProvider::resetChatProviders);

    connect(this, &ChatMessageSearchProvider::chatRoomAdded, this, [this](QString roomUid) {
        auto room = m_chatRoomsByUid.value(roomUid);
        auto context = m_chatRoomContextsByUid.value(roomUid);
        if (room && context) {
            QList<ChatMessageSearchIndexer::Message> messages;

            auto chatMessages = room->chatMessages();
            for (auto &chatMessage : chatMessages) {
                if (chatMessage && chatMessage->content()) {
                    if (const auto textContent =
                                qobject_cast<ChatMessageContentText *>(chatMessage->content())) {
                        messages.append(
                                { chatMessage->eventId(), roomUid, textContent->rawText() });
                    }
                }
            }

            if (!messages.isEmpty()) {
                if (!ChatMessageSearchIndexer::instance().addMessages(messages)) {
                    qCWarning(lcChatMessageSearchProvider)
                            << "Failed to add messages to indexer:"
                            << ChatMessageSearchIndexer::instance().lastError();
                };
                ChatMessageSearchIndexer::instance().optimize();
            }

            // Connect to chat room changes as long as the associated context lives
            connect(room, &IChatRoom::chatMessageAdded, context,
                    [roomUid](qsizetype, ChatMessage *chatMessage) {
                        if (chatMessage && chatMessage->content()) {
                            if (const auto textContent = qobject_cast<ChatMessageContentText *>(
                                        chatMessage->content())) {
                                ChatMessageSearchIndexer::instance().addMessage(
                                        { chatMessage->eventId(), roomUid,
                                          textContent->rawText() });
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
                    [roomUid](qsizetype, ChatMessage *chatMessage) {
                        if (chatMessage && chatMessage->content()) {
                            if (const auto textContent = qobject_cast<ChatMessageContentText *>(
                                        chatMessage->content())) {
                                ChatMessageSearchIndexer::instance().updateMessage(
                                        { chatMessage->eventId(), roomUid,
                                          textContent->rawText() });
                            }
                        }
                    });
            connect(room, &IChatRoom::chatMessagesReset, context, [roomUid]() {
                ChatMessageSearchIndexer::instance().removeMessagesByRoom(roomUid);
            });
        }
    });

    connect(this, &ChatMessageSearchProvider::chatRoomDeleted, this, [](QString roomUid) {
        ChatMessageSearchIndexer::instance().removeMessagesByRoom(roomUid);
    });

    connect(this, &ChatMessageSearchProvider::searchPhraseChanged, this, [this]() {
        if (m_model->rowCount() > 0) {
            m_model->reset();
        }

        if (m_searchPhrase.size() >= 3) {
            auto results = ChatMessageSearchIndexer::instance().search(m_searchPhrase);
            if (results.isEmpty()) {
                qCWarning(lcChatMessageSearchProvider)
                        << "Message search did not return any results:"
                        << ChatMessageSearchIndexer::instance().lastError();
            } else {
                m_model->addResults(results);
            }
        }
    });

    resetChatProviders();
}

void ChatMessageSearchProvider::resetChatProviders()
{
    qCWarning(lcChatMessageSearchProvider) << "Reloading chat providers";

    // TODO: Clear SQLite tables

    m_chatRoomsByUid.clear();
    qDeleteAll(m_chatRoomContextsByUid);
    m_chatRoomContextsByUid.clear();

    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }

    m_chatProviders = ChatConnectorManager::instance().chatConnectors();
    if (m_chatProviders.isEmpty()) {
        qCWarning(lcChatMessageSearchProvider) << "No chat providers found";
    } else {
        m_chatProviderContext = new QObject(this);

        for (auto provider : std::as_const(m_chatProviders)) {
            if (!provider) {
                continue;
            }

            const auto roomCount = provider->chatRoomsCount();
            for (int i = 0; i < roomCount; i++) {
                auto room = provider->chatRoomByIndex(i);
                if (room) {
                    QString roomUid = room->id();

                    m_chatRoomsByUid.insert(roomUid, room);
                    m_chatRoomContextsByUid.insert(roomUid, new QObject(this));

                    Q_EMIT chatRoomAdded(roomUid);
                }
            }

            // Connect to chat provider changes as long as the shared context lives
            connect(provider, &IChatProvider::chatRoomAdded, m_chatProviderContext,
                    [this](qsizetype, IChatRoom *room, QString) {
                        if (room) {
                            QString roomUid = room->id();

                            m_chatRoomsByUid.insert(roomUid, room);
                            m_chatRoomContextsByUid.insert(roomUid, new QObject(this));

                            Q_EMIT chatRoomAdded(roomUid);
                        }
                    });
            connect(provider, &IChatProvider::chatRoomRemoved, m_chatProviderContext,
                    [this](qsizetype, IChatRoom *room) {
                        if (room) {
                            QString roomUid = room->id();

                            if (m_chatRoomsByUid.remove(roomUid)) {
                                m_chatRoomContextsByUid.take(roomUid)->deleteLater();

                                Q_EMIT chatRoomDeleted(roomUid);
                            }
                        }
                    });
            // TODO: What about renamed chat rooms?
        }
    }
}

QString ChatMessageSearchProvider::getChatMessageText(const QString &roomUid,
                                                      const QString &messageUid)
{
    auto room = m_chatRoomsByUid.value(roomUid);
    if (!room) {
        return "";
    }

    auto message = room->chatMessageById(messageUid);
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
    // TODO: Clear SQLite tables

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
