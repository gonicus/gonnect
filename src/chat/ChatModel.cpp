#include "ChatModel.h"
#include "EmojiResolver.h"
#include "ChatMessage.h"
#include <QRegularExpression>

ChatModel::ChatModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatModel::chatRoomChanged, this, &ChatModel::onChatRoomChanged);
}

QHash<int, QByteArray> ChatModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::EventId), "eventId" },
        { static_cast<int>(Roles::FromId), "fromId" },
        { static_cast<int>(Roles::NickName), "nickName" },
        { static_cast<int>(Roles::Message), "message" },
        { static_cast<int>(Roles::Timestamp), "timestamp" },
        { static_cast<int>(Roles::ImageUrl), "imageUrl" },
        { static_cast<int>(Roles::IsPrivateMessage), "isPrivateMessage" },
        { static_cast<int>(Roles::IsOwnMessage), "isOwnMessage" },
        { static_cast<int>(Roles::IsSystemMessage), "isSystemMessage" },
    };
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_chatRoom ? m_chatRoom->chatMessages().size() : 0;
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (!m_chatRoom) {
        return QVariant();
    }

    const auto item = m_chatRoom->chatMessages().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::EventId):
        return item->eventId();
    case static_cast<int>(Roles::FromId):
        return item->fromId();
    case static_cast<int>(Roles::NickName):
        return item->nickName();
    case static_cast<int>(Roles::Message): {
        if (item->flags() & ChatMessage::Flag::Markdown) {
            return EmojiResolver::instance().replaceEmojiCodes(item->message());
        } else {
            return addLinkTags(EmojiResolver::instance().replaceEmojiCodes(item->message()));
        }
    }
    case static_cast<int>(Roles::ImageUrl):
        return ""; // TODO
    case static_cast<int>(Roles::Timestamp):
        return item->timestamp();
    case static_cast<int>(Roles::IsPrivateMessage):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::PrivateMessage);
    case static_cast<int>(Roles::IsOwnMessage):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::OwnMessage);
    case static_cast<int>(Roles::IsSystemMessage):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::SystemMessage);
    default:
        return QVariant();
    }
}

void ChatModel::onChatRoomChanged()
{
    beginResetModel();

    if (m_chatRoomContext) {
        m_chatRoomContext->deleteLater();
        m_chatRoomContext = nullptr;
    }

    if (m_chatRoom) {
        m_chatRoomContext = new QObject(this);
        connect(m_chatRoom, &IChatRoom::chatMessageAdded, m_chatRoomContext,
                [this](qsizetype index, ChatMessage *) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                    updateRealMessagesCount();
                });
        connect(m_chatRoom, &IChatRoom::chatMessagesReset, m_chatRoomContext, [this]() {
            beginResetModel();
            endResetModel();
            updateRealMessagesCount();
        });
    }

    endResetModel();
    updateRealMessagesCount();
}

void ChatModel::updateRealMessagesCount()
{
    uint count = 0;

    if (m_chatRoom) {
        const auto &messages = m_chatRoom->chatMessages();
        for (const auto &message : messages) {
            if (!(message->flags() & ChatMessage::Flag::SystemMessage)) {
                ++count;
            }
        }
    }

    if (m_realMessagesCount != count) {
        m_realMessagesCount = count;
        emit realMessagesCountChanged();
    }
}

QString ChatModel::addLinkTags(const QString &orig) const
{
    static const QRegularExpression re("^\\S+\\.\\S{2,}[^.]$",
                                       QRegularExpression::CaseInsensitiveOption);

    auto split = orig.split(' ');

    QMutableListIterator it(split);
    while (it.hasNext()) {
        auto s = it.next();

        const auto match = re.match(s);
        if (match.hasMatch()) {
            it.setValue(QString("<a href=\"%1\">%1</a>").arg(s));
        }
    }

    return split.join(' ');
}
