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
    roles[static_cast<int>(Roles::IsLatestOwnMessage)] = "isLatestOwnMessage";
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
    case static_cast<int>(Roles::IsLatestOwnMessage): {
        const QModelIndex sourceIndex = mapToSource(index);
        if (!isValidOwnMessage(sourceIndex)) {
            return false;
        }

        const auto *chatRoom = model->chatRoom();
        const auto messages = chatRoom->chatMessages();
        const auto *currMsg = messages.at(sourceIndex.row());
        const auto currReadUsers = readUsersFor(chatRoom, currMsg);

        // Find next newer own message
        for (qsizetype i = index.row() - 1; i >= 0; --i) {
            if (messageAt(i)) {
                return false;
            }
        }
        return true;
    }
    case static_cast<int>(Roles::ReadUsers): {

        // readUsers shall be shown/returned if this is the latest own message or if it is an own
        // message and readUsers are different from the previous own message.

        QVariantList result;

        const QModelIndex sourceIndex = mapToSource(index);
        if (!isValidOwnMessage(sourceIndex)) {
            return result;
        }

        const auto *chatRoom = q_check_ptr(model->chatRoom());
        const auto messages = chatRoom->chatMessages();
        const auto *currMsg = q_check_ptr(messages.at(sourceIndex.row()));
        const auto currReadUsers = readUsersFor(chatRoom, currMsg);

        for (qsizetype i = index.row() - 1; i >= 0; --i) {
            if (auto *message = messageAt(i);
                message && currReadUsers == readUsersFor(chatRoom, message)) {
                return result;
            }
        }

        // Build result list
        result.reserve(currReadUsers.size());
        std::ranges::transform(currReadUsers, std::back_inserter(result),
                               [](ChatUser *user) { return QVariant::fromValue(user); });
        return result;
    }

    default:
        return QSortFilterProxyModel::data(index, role);
    }
}

ChatMessage *ChatProxyModel::messageAt(qsizetype proxyIndex) const
{
    const QModelIndex prevSource = mapToSource(this->index(proxyIndex, 0));
    if (!isValidOwnMessage(prevSource)) {
        return nullptr;
    }
    return q_check_ptr(qobject_cast<ChatModel *>(sourceModel()))
            ->chatRoom()
            ->chatMessages()
            .at(prevSource.row());
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

bool ChatProxyModel::isValidOwnMessage(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return false;
    }

    const auto *model = qobject_cast<ChatModel *>(sourceModel());
    if (!model) {
        return false;
    }

    const auto *chatRoom = model->chatRoom();
    if (!chatRoom) {
        return false;
    }

    const auto &messages = chatRoom->chatMessages();
    if (messages.isEmpty()) {
        return false;
    }

    const auto row = index.row();
    if (row < 0 || row >= messages.size()) {
        return false;
    }

    const auto *currMsg = messages.at(row);
    if (!(currMsg->flags() & ChatMessage::Flag::OwnMessage)) {
        return false;
    }
    if (currMsg->flags() & (ChatMessage::Flag::Pending | ChatMessage::Flag::Failed)) {
        return false;
    }

    return true;
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

        connect(chatRoom, &IChatRoom::chatMessageAdded, m_chatRoomContext,
                [this]() { invalidateProxyRoles(); });
        connect(chatRoom, &IChatRoom::chatMessageRemoved, m_chatRoomContext,
                [this]() { invalidateProxyRoles(); });
        connect(chatRoom, &IChatRoom::readMarkersChanged, m_chatRoomContext,
                [this]() { invalidateProxyRoles(); });
    }
}

void ChatProxyModel::invalidateProxyRoles()
{
    const auto rows = rowCount(QModelIndex());
    if (rows > 0) {
        Q_EMIT dataChanged(index(0, 0), index(rows - 1, 0),
                           { static_cast<int>(Roles::ReadUsers),
                             static_cast<int>(Roles::IsLatestOwnMessage) });
    }
}
