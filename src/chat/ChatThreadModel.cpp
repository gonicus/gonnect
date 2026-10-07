#include "ChatThreadModel.h"
#include "IChatRoom.h"
#include "ChatMessage.h"

static constexpr int maxAgeDays = 7;

ChatThreadModel::ChatThreadModel(QObject *parent) : QAbstractListModel{ parent } { }

QHash<int, QByteArray> ChatThreadModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::ThreadId), "threadId" },
        { static_cast<int>(Roles::FirstMessage), "firstMessage" },
        { static_cast<int>(Roles::LastMessage), "lastMessage" },
        { static_cast<int>(Roles::UnreadCount), "unreadCount" },
    };
}

int ChatThreadModel::rowCount(const QModelIndex &) const
{
    return m_threadSummaries.length();
}

QVariant ChatThreadModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const auto &summary = m_threadSummaries.at(index.row());

    switch (role) {
    case static_cast<int>(Roles::ThreadId):
        return summary.threadId;

    case static_cast<int>(Roles::UnreadCount):
        return summary.unreadCount;

    case static_cast<int>(Roles::FirstMessage):
        return QVariant::fromValue<ChatMessage *>(summary.firstMessage);

    case static_cast<int>(Roles::LastMessage):
        return QVariant::fromValue<ChatMessage *>(summary.lastMessage);

    default:
        return QVariant();
    }
}

void ChatThreadModel::setChatRoom(IChatRoom *room)
{
    if (m_chatRoom != room) {
        m_chatRoom = room;
        onChatRoomChanged();
        Q_EMIT chatRoomChanged();
    }
}

void ChatThreadModel::onChatRoomChanged()
{

    if (m_chatRoomContext) {
        delete m_chatRoomContext;
        m_chatRoomContext = nullptr;
    }

    if (m_chatRoom) {
        m_chatRoomContext = new QObject(this);
        connect(m_chatRoom, &QObject::destroyed, m_chatRoomContext, [this](QObject *obj) {
            if (m_chatRoom == obj) {
                setChatRoom(nullptr);
            }
        });

        connect(m_chatRoom, &IChatRoom::chatMessageAdded, m_chatRoomContext,
                [this]() { rebuildThreadSummaries(); });
        connect(m_chatRoom, &IChatRoom::chatMessageRemoved, m_chatRoomContext,
                [this]() { rebuildThreadSummaries(); });
        connect(m_chatRoom, &IChatRoom::chatMessageFlagsChanged, m_chatRoomContext,
                [this]() { rebuildThreadSummaries(); });
        connect(m_chatRoom, &IChatRoom::chatMessageContentChanged, m_chatRoomContext,
                [this]() { rebuildThreadSummaries(); });
        connect(m_chatRoom, &IChatRoom::chatMessagesReset, m_chatRoomContext,
                [this]() { rebuildThreadSummaries(); });
        connect(m_chatRoom, &IChatRoom::readMarkersChanged, m_chatRoomContext,
                [this]() { rebuildThreadSummaries(); });
        connect(m_chatRoom, &IChatRoom::ownLastReadTimestampChanged, m_chatRoomContext,
                [this]() { rebuildThreadSummaries(); });

        rebuildThreadSummaries();
    }
}

void ChatThreadModel::rebuildThreadSummaries()
{
    beginResetModel();

    m_threadSummaries.clear();
    if (!m_chatRoom) {
        endResetModel();
        return;
    }

    // Utilize the fact that chat messages are sorted
    // -> first appearance of thread id is automatically the newest message of that thread
    // threadId = messageId/eventId of thread root message

    const auto chatMessages = m_chatRoom->chatMessages();
    QSet<QString> handledThreadIds;
    const auto maxOldTimeStamp = QDateTime::currentDateTime().addDays(-maxAgeDays);

    for (auto *msg : chatMessages | std::views::reverse) {
        const auto threadId = msg->threadId();
        if (threadId.isEmpty() || handledThreadIds.contains(threadId)) {
            continue;
        }
        if (msg->timestamp() < maxOldTimeStamp) {
            continue;
        }
        handledThreadIds.insert(threadId);
        auto *rootMsg = m_chatRoom->messageById(threadId);
        m_threadSummaries.append({ threadId, msg, rootMsg, 0 });
    }

    endResetModel();
}
