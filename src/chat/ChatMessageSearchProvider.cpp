#include <QLoggingCategory>

#include "ChatMessageSearchProvider.h"
#include "ChatMessageSearchIndexer.h"
#include "IpcChatRoom.h"

#include "ChatConnectorManager.h"
#include "ChatMessage.h"

Q_LOGGING_CATEGORY(lcChatMessageSearchProvider, "gonnect.chat.message.search.provider")

using namespace std::chrono_literals;

ChatMessageSearchProvider::ChatMessageSearchProvider(QObject *parent) : QObject{ parent }
{
    m_backfillTimer.setSingleShot(false);
    m_backfillTimer.setInterval(10s);
    m_backfillTimer.callOnTimeout(this, &ChatMessageSearchProvider::onBackfillTick);

    m_model = new ChatMessageSearchModel(this);
    if (!m_model) {
        return;
    }

    m_indexer = new ChatMessageSearchIndexer(this);
    if (!m_indexer) {
        return;
    }

    connect(m_indexer, &ChatMessageSearchIndexer::initialized, this, [this]() {
        connect(&ChatConnectorManager::instance(), &ChatConnectorManager::chatConnectorsChanged,
                this, &ChatMessageSearchProvider::updateChatProviders);

        updateChatProviders();
    });

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

QString ChatMessageSearchProvider::indexingSettingsKey(const QString &roomUid) const
{
    return QString("chatSearchIndexing/%1").arg(roomUid);
}

QString ChatMessageSearchProvider::backfillSettingsKey(const QString &roomUid) const
{
    return QString("chatSearchBackfillComplete/%1").arg(roomUid);
}

void ChatMessageSearchProvider::indexRoomMessages(IChatRoom &room) const
{
    const auto chatMessages = room.chatMessages();
    QList<ChatMessageSearchIndexer::Message> messages;
    messages.reserve(chatMessages.size());

    for (auto &chatMessage : chatMessages) {
        if (chatMessage && chatMessage->content()) {
            if (const auto textContent =
                        qobject_cast<ChatMessageContentText *>(chatMessage->content())) {
                messages.append({ chatMessage->eventId(), room.id(), textContent->rawText(),
                                  chatMessage->timestamp().toMSecsSinceEpoch() });
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
}

void ChatMessageSearchProvider::clearRoomIndex(const QString &roomUid)
{
    if (roomUid.isEmpty()) {
        return;
    }
    m_indexer->removeMessagesByRoom(roomUid);
    setRoomBackfillComplete(roomUid, false);
}

IChatProvider *ChatMessageSearchProvider::getChatProviderByRoom(const QString &roomUid)
{
    auto rit = m_chatRoomsByUid.find(roomUid);
    if (rit == m_chatRoomsByUid.end()) {
        return nullptr;
    }

    RoomConnection &rconn = rit.value();
    QString providerUid = rconn.providerUid;

    auto pit = m_chatProvidersByUid.find(providerUid);
    if (pit == m_chatProvidersByUid.end()) {
        return nullptr;
    }

    ProviderConnection &conn = pit.value();
    return conn.provider;
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
    QMutableHashIterator<QString, ProviderConnection> it(m_chatProvidersByUid);
    while (it.hasNext()) {
        it.next();
        QString providerUid = it.key();

        if (!activeIds.contains(providerUid)) {
            removeChatRoomsByProvider(providerUid);

            it.remove();
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
        loadChatRoom(providerUid, provider->chatRoomByIndex(i));
    }

    // Connect to chat provider changes as long as the associated context lives
    connect(provider, &IChatProvider::chatRoomAdded, context,
            [providerUid, this](qsizetype, IChatRoom *room, QString) {
                loadChatRoom(providerUid, room);
            });
    connect(provider, &IChatProvider::chatRoomRemoved, context,
            [this](qsizetype, IChatRoom *room) { removeChatRoom(room); });

    // TODO: Do chatRoomJoined/chatRoomLeft also emit added/removed?
}

void ChatMessageSearchProvider::loadChatRoom(const QString &providerUid, IChatRoom *room)
{
    if (!room) {
        return;
    }

    QString roomUid = room->id();

    m_chatRoomsByUid.emplace(roomUid, providerUid, room);

    auto it = m_chatRoomsByUid.find(roomUid);
    if (it == m_chatRoomsByUid.end()) {
        return;
    }

    RoomConnection &conn = it.value();
    QObject *context = conn.context.get();
    if (!context) {
        return;
    }

    if (isRoomIndexingEnabled(roomUid)) {
        indexRoomMessages(*room);
    }

    // Connect to chat room changes as long as the associated context lives
    connect(room, &IChatRoom::chatMessageAdded, context,
            [this, roomUid](qsizetype, ChatMessage *chatMessage) {
                if (isRoomIndexingEnabled(roomUid) && chatMessage && chatMessage->content()) {
                    if (const auto textContent =
                                qobject_cast<ChatMessageContentText *>(chatMessage->content())) {
                        m_indexer->addMessage({ chatMessage->eventId(), roomUid,
                                                textContent->simpleText(),
                                                chatMessage->timestamp().toMSecsSinceEpoch() });
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
                if (isRoomIndexingEnabled(roomUid) && chatMessage && chatMessage->content()) {
                    if (const auto textContent =
                                qobject_cast<ChatMessageContentText *>(chatMessage->content())) {
                        m_indexer->updateMessage({ chatMessage->eventId(), roomUid,
                                                   textContent->simpleText(),
                                                   chatMessage->timestamp().toMSecsSinceEpoch() });
                    }
                }
            });
    connect(room, &IChatRoom::chatMessageOutOfSequenceReceived, context,
            [this, roomUid](ChatMessage *chatMessage) {
                if (chatMessage) {
                    Q_EMIT chatMessageLoaded(roomUid, chatMessage->eventId());
                }
            });
    connect(room, &IChatRoom::chatMessagesReset, context,
            [this, roomUid]() { m_indexer->removeMessagesByRoom(roomUid); });

    if (!m_backfillTimer.isActive()) {
        m_backfillTimer.start();
    }
}

void ChatMessageSearchProvider::removeChatRoom(IChatRoom *room)
{
    if (!room) {
        return;
    }

    const QString roomUid = room->id();

    m_chatRoomsByUid.remove(roomUid);

    clearRoomIndex(roomUid);
}

void ChatMessageSearchProvider::removeChatRoomsByProvider(const QString &providerUid)
{
    QMutableHashIterator<QString, RoomConnection> it(m_chatRoomsByUid);
    while (it.hasNext()) {
        it.next();
        const QString roomUid = it.key();

        if (it.value().providerUid == providerUid) {
            it.remove();

            clearRoomIndex(roomUid);
        }
    }
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

    ChatMessage *chatMessage = room->chatMessageById(messageUid);
    if (!chatMessage) {
        if (auto *ipcRoom = qobject_cast<IpcChatRoom *>(room)) {
            ipcRoom->ensureMessageLoaded(messageUid);
        }
    }

    return chatMessage;
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

bool ChatMessageSearchProvider::isRoomIndexingEnabled(const QString &roomUid) const
{
    if (roomUid.isEmpty()) {
        return false;
    }
    return m_appSettings.value(indexingSettingsKey(roomUid), false).toBool();
}

void ChatMessageSearchProvider::setRoomIndexingEnabled(const QString &roomUid, bool enabled)
{
    if (roomUid.isEmpty()) {
        return;
    }

    const bool oldValue = isRoomIndexingEnabled(roomUid);
    if (oldValue != enabled) {
        m_appSettings.setValue(indexingSettingsKey(roomUid), enabled);

        if (enabled) {
            const auto roomConn = m_chatRoomsByUid.value(roomUid);
            if (roomConn.room) {
                indexRoomMessages(*roomConn.room);
            }
            if (!m_backfillTimer.isActive()) {
                m_backfillTimer.start();
            }
        } else {
            clearRoomIndex(roomUid);
        }

        Q_EMIT roomIndexingEnabledChanged(roomUid, enabled);
    }
}

bool ChatMessageSearchProvider::isRoomBackfillComplete(const QString &roomUid) const
{
    if (roomUid.isEmpty()) {
        return false;
    }
    return m_appSettings.value(backfillSettingsKey(roomUid), false).toBool();
}

void ChatMessageSearchProvider::setRoomBackfillComplete(const QString &roomUid, bool isComplete)
{
    qCInfo(lcChatMessageSearchProvider).noquote().nospace()
            << "setRoomBackfillComplete(" << roomUid << ", " << isComplete << ")";

    if (roomUid.isEmpty()) {
        qCWarning(lcChatMessageSearchProvider) << "roomUid is empty";
        return;
    }

    const bool oldValue = isRoomBackfillComplete(roomUid);
    if (oldValue != isComplete) {
        m_appSettings.setValue(backfillSettingsKey(roomUid), isComplete);
    }
}

static constexpr qsizetype MAX_BACKFILL_MESSAGE_COUNT = 100000;
static constexpr auto MAX_BACKFILL_AGE = std::chrono::days(365);
static constexpr quint32 BACKFILL_PAGE_SIZE = 500;

void ChatMessageSearchProvider::onBackfillTick()
{
    IChatRoom *candidate = nullptr;

    // Set room states and find backfill candidate
    for (const auto &roomConn : std::as_const(m_chatRoomsByUid)) {
        if (auto *room = roomConn.room) {
            const auto &roomId = room->id();

            if (isRoomIndexingEnabled(roomId) && !isRoomBackfillComplete(roomId)) {
                const auto oldestTimestamp = m_indexer->oldestIndexedTimestampForRoom(roomId);
                const auto cutoff = QDateTime::currentDateTime()
                                            .addDays(-MAX_BACKFILL_AGE.count())
                                            .toMSecsSinceEpoch();

                if (room->isCompletelyLoaded()
                    || m_indexer->indexedMessageCountForRoom(roomId) >= MAX_BACKFILL_MESSAGE_COUNT
                    || (oldestTimestamp > 0 && oldestTimestamp < cutoff)) {

                    if (room->isCompletelyLoaded()) {
                        qCInfo(lcChatMessageSearchProvider)
                                << "Backfill complete (end of history) for" << roomId;
                    } else if (m_indexer->indexedMessageCountForRoom(roomId)
                               >= MAX_BACKFILL_MESSAGE_COUNT) {
                        qCInfo(lcChatMessageSearchProvider)
                                << "Backfill complete (count cap) for" << roomId;
                    } else if (oldestTimestamp > 0 && oldestTimestamp < cutoff) {
                        qCInfo(lcChatMessageSearchProvider)
                                << "Backfill complete (age cap) for" << roomId;
                    }
                    setRoomBackfillComplete(roomId, true);

                } else if (!candidate && !room->isLoadingMessageHistory()
                           && room->ownUserJoinState() == IChatRoom::UserRoomState::Joined) {
                    candidate = room;
                }
            }
        }
    }

    // New backfill
    if (candidate) {
        QString anchor;
        if (candidate->chatMessages().isEmpty()) {
            anchor = m_indexer->oldestIndexedMessageUid(candidate->id());
        }

        qCInfo(lcChatMessageSearchProvider)
                << "Backfill: requesting page for room" << candidate->id() << candidate->name()
                << "and anchor" << anchor;
        candidate->loadMessages(QString(), anchor, BACKFILL_PAGE_SIZE);

    } else {
        m_backfillTimer.stop();
    }
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
