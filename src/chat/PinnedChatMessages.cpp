#include "PinnedChatMessages.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPinnedChatMessages, "gonnect.app.chat.PinnedChatMessages")

PinnedChatMessages::PinnedChatMessages(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &PinnedChatMessages::chatRoomChanged, this,
            &PinnedChatMessages::onChatRoomUpdated);
}

QHash<int, QByteArray> PinnedChatMessages::roleNames() const
{
    return {
        { static_cast<int>(Roles::EventId), "eventId" },
        { static_cast<int>(Roles::NickName), "nickName" },
        { static_cast<int>(Roles::Content), "content" },
    };
}

int PinnedChatMessages::rowCount(const QModelIndex &) const
{
    return m_chatRoom ? m_chatRoom->pinnedChatMessageCount() : 0;
}

QVariant PinnedChatMessages::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_chatRoom) {
        return QVariant();
    }

    const ChatMessage *message = m_chatRoom->pinnedChatMessageByIndex(index.row());

    if (!message) {
        qCWarning(lcPinnedChatMessages) << "Unable to get ChatMessage for index" << index.row();
        return QVariant();
    }

    switch (role) {
    case static_cast<int>(Roles::EventId):
        return message->eventId();

    case static_cast<int>(Roles::Content):
        return QVariant::fromValue(message->content());

    case static_cast<int>(Roles::NickName):
    default:
        return message->nickName();
    }
}

void PinnedChatMessages::onChatRoomUpdated()
{
    beginResetModel();

    if (m_chatRoomContext) {
        delete m_chatRoomContext;
        m_chatRoomContext = nullptr;
    }

    if (m_chatRoom) {
        m_chatRoomContext = new QObject(this);
        connect(m_chatRoom, &IChatRoom::pinnedMessagesChanged, m_chatRoomContext, [this]() {
            beginResetModel();
            endResetModel();
        });
        connect(m_chatRoom, &IChatRoom::chatMessageContentChanged, m_chatRoomContext,
                [this](qsizetype, ChatMessage *chatMessage) {
                    if (!chatMessage || !m_chatRoom) {
                        return;
                    }

                    const auto pinnedIndex = m_chatRoom->indexOfPinnedChatMessage(chatMessage);
                    if (pinnedIndex < 0 || pinnedIndex >= m_chatRoom->pinnedChatMessageCount()) {
                        // Message is not pinned
                        return;
                    }

                    const auto idx = createIndex(pinnedIndex, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::Content) });
                });
        connect(m_chatRoom, &QObject::destroyed, m_chatRoomContext, [this](QObject *obj) {
            auto *destroyedRoom = qobject_cast<IChatRoom *>(obj);
            if (destroyedRoom && m_chatRoom == destroyedRoom) {
                setProperty("chatRoom", QVariant::fromValue(nullptr));
            }
        });
    }

    endResetModel();
}
