#include "ChatProxyModel.h"
#include "ChatModel.h"

ChatProxyModel::ChatProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    sort(0);

    connect(this, &QSortFilterProxyModel::sourceModelChanged, this,
            &ChatProxyModel::onSourceModelChanged);
    onSourceModelChanged();
}

QHash<int, QByteArray> ChatProxyModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    if (const auto model = sourceModel()) {
        roles = model->roleNames();
    }
    roles[static_cast<int>(Roles::ReadUsers)] = "readUsers";
    return roles;
}

QVariant ChatProxyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const auto *model = qobject_cast<ChatModel *>(sourceModel());
    if (!model) {
        return QVariant();
    }

    switch (role) {
    case static_cast<int>(Roles::ReadUsers): {

        QVariantList result;

        const auto *chatRoom = model->chatRoom();
        if (!chatRoom) {
            return result;
        }

        const QModelIndex sourceIndex = mapToSource(index);
        if (!sourceIndex.isValid()) {
            return result;
        }

        const auto messages = chatRoom->chatMessages();
        if (messages.isEmpty()) {
            return result;
        }

        // Get read users of the previous row. Only produce result if it differs (UI optimization).
        const auto currReadUsers = readUsersFor(chatRoom, messages.at(sourceIndex.row()));

        const auto row = index.row();
        if (row > 0) {
            const QModelIndex prevIndex = mapToSource(this->index(row - 1, index.column()));
            if (!prevIndex.isValid()) {
                return result;
            }

            const auto prevReadUsers = readUsersFor(chatRoom, messages.at(prevIndex.row()));
            if (currReadUsers == prevReadUsers) {
                return result;
            }
        }

        result.reserve(currReadUsers.size());

        std::ranges::transform(currReadUsers, std::back_inserter(result),
                               [](ChatUser *user) { return QVariant::fromValue(user); });

        return result;
    }

    default:
        return QSortFilterProxyModel::data(index, role);
    }
}

QList<ChatUser *> ChatProxyModel::readUsersFor(const IChatRoom *chatRoom,
                                               const ChatMessage *message) const
{
    QList<ChatUser *> users;

    const auto messageTime = message->timestamp();
    for (auto *user : chatRoom->chatUsers()) {
        const auto readTime = chatRoom->lastReadTimestamp(user->id());
        if (readTime.isValid() && readTime >= messageTime
            && chatRoom->chatUserRoomState(user) == IChatRoom::UserRoomState::Joined) {
            users.append(user);
        }
    }

    std::ranges::sort(users, [](const ChatUser *left, const ChatUser *right) -> bool {
        return left->displayName().localeAwareCompare(right->displayName()) < 0;
    });

    return users;
}

bool ChatProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    using Roles = ChatModel::Roles;

    const auto leftTime = model->data(sourceLeft, static_cast<int>(Roles::Timestamp)).toDateTime();
    const auto rightTime =
            model->data(sourceRight, static_cast<int>(Roles::Timestamp)).toDateTime();

    return leftTime > rightTime;
}

void ChatProxyModel::onSourceModelChanged()
{
    if (m_sourceModelContext) {
        delete m_sourceModelContext;
        m_sourceModelContext = nullptr;
    }

    const auto *model = qobject_cast<ChatModel *>(sourceModel());
    if (model) {
        m_sourceModelContext = new QObject(this);
        connect(model, &ChatModel::chatRoomChanged, m_sourceModelContext,
                [this]() { onChatRoomChanged(); });
    }

    onChatRoomChanged();
}

void ChatProxyModel::onChatRoomChanged()
{
    if (m_chatRoomContext) {
        delete m_chatRoomContext;
        m_chatRoomContext = nullptr;
    }

    const auto *model = qobject_cast<ChatModel *>(sourceModel());
    if (!model) {
        return;
    }

    if (auto *chatRoom = model->chatRoom()) {
        m_chatRoomContext = new QObject(this);

        connect(chatRoom, &IChatRoom::readMarkersChanged, m_chatRoomContext, [this]() {
            const auto rows = rowCount(QModelIndex());
            if (rows > 0) {
                Q_EMIT dataChanged(index(0, 0), index(rows - 1, 0),
                                   { static_cast<int>(Roles::ReadUsers) });
            }
        });
    }
}
