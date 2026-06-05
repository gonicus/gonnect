#include "ChatModel.h"
#include "EmojiResolver.h"
#include "ChatMessage.h"
#include "ChatMessageReaction.h"
#include "IChatProvider.h"
#include "ChatMessageContentImage.h"
#include "ChatMessageContentText.h"
#include "ChatMessageContentFile.h"
#include "ChatMessageContentVideoFile.h"
#include "ChatMessageContentUserStateChange.h"
#include <QRegularExpression>

ChatModel::ChatModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatModel::chatRoomChanged, this, &ChatModel::onChatRoomChanged);
}

QHash<int, QByteArray> ChatModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::EventId), "eventId" },
        { static_cast<int>(Roles::RoomId), "roomId" },
        { static_cast<int>(Roles::FromId), "fromId" },
        { static_cast<int>(Roles::NickName), "nickName" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::UserState), "userState" },
        { static_cast<int>(Roles::AffectedUserId), "affectedUserId" },
        { static_cast<int>(Roles::SimpleText), "simpleText" },
        { static_cast<int>(Roles::MultiText), "multiText" },
        { static_cast<int>(Roles::Timestamp), "timestamp" },
        { static_cast<int>(Roles::ImageUrl), "imageUrl" },
        { static_cast<int>(Roles::FileUrl), "fileUrl" },
        { static_cast<int>(Roles::FileName), "fileName" },
        { static_cast<int>(Roles::FileSize), "fileSize" },
        { static_cast<int>(Roles::ThumbnailFileUrl), "thumbnailFileUrl" },
        { static_cast<int>(Roles::Reactions), "reactions" },

        { static_cast<int>(Roles::IsPrivateMessage), "isPrivateMessage" },
        { static_cast<int>(Roles::IsOwnMessage), "isOwnMessage" },
        { static_cast<int>(Roles::IsSystemMessage), "isSystemMessage" },
        { static_cast<int>(Roles::IsEncrypted), "isEncrypted" },
        { static_cast<int>(Roles::IsPinned), "isPinned" },
        { static_cast<int>(Roles::IsSameUserAsPrevious), "isSameUserAsPrevious" },
        { static_cast<int>(Roles::IsSameMinuteAsPrevious), "isSameMinuteAsPrevious" },
        { static_cast<int>(Roles::IsSameDayAsPrevious), "isSameDayAsPrevious" },
        { static_cast<int>(Roles::IsStateUpdate), "isStateUpdate" },
        { static_cast<int>(Roles::IsText), "isText" },
        { static_cast<int>(Roles::IsSimpleText), "isSimpleText" },
        { static_cast<int>(Roles::IsMultiText), "isMultiText" },
        { static_cast<int>(Roles::IsImage), "isImage" },
        { static_cast<int>(Roles::IsFile), "isFile" },
        { static_cast<int>(Roles::IsAudioFile), "isAudioFile" },
        { static_cast<int>(Roles::IsVideoFile), "isVideoFile" },

        { static_cast<int>(Roles::HasRelatedMessage), "hasRelatedMessage" },
        { static_cast<int>(Roles::RelatedMessageNickName), "relatedMessageNickName" },
        { static_cast<int>(Roles::RelatedMessageIsStateUpdate), "relatedMessageIsStateUpdate" },
        { static_cast<int>(Roles::RelatedMessageIsText), "relatedMessageIsText" },
        { static_cast<int>(Roles::RelatedMessageIsSimpleText), "relatedMessageIsSimpleText" },
        { static_cast<int>(Roles::RelatedMessageIsMultiText), "relatedMessageIsMultiText" },
        { static_cast<int>(Roles::RelatedMessageIsImage), "relatedMessageIsImage" },
        { static_cast<int>(Roles::RelatedMessageIsFile), "relatedMessageIsFile" },
        { static_cast<int>(Roles::RelatedMessageIsAudioFile), "relatedMessageIsAudioFile" },
        { static_cast<int>(Roles::RelatedMessageIsVideoFile), "relatedMessageIsVideoFile" },
        { static_cast<int>(Roles::RelatedMessageUserState), "relatedMessageUserState" },
        { static_cast<int>(Roles::RelatedMessageAffectedUserId), "relatedMessageAffectedUserId" },
        { static_cast<int>(Roles::RelatedMessageSimpleText), "relatedMessageSimpleText" },
        { static_cast<int>(Roles::RelatedMessageMultiText), "relatedMessageMultiText" },
        { static_cast<int>(Roles::RelatedMessageImageUrl), "relatedMessageImageUrl" },
        { static_cast<int>(Roles::RelatedMessageFileUrl), "relatedMessageFileUrl" },
        { static_cast<int>(Roles::RelatedMessageFileName), "relatedMessageFileName" },
        { static_cast<int>(Roles::RelatedMessageFileSize), "relatedMessageFileSize" },
        { static_cast<int>(Roles::RelatedMessageThumbnailFileUrl),
          "relatedMessageThumbnailFileUrl" },

        { static_cast<int>(Roles::MentionedUserNames), "mentionedUserNames" },
    };
}

int ChatModel::toNormalRole(const int role)
{
    return static_cast<int>(toNormalRole(static_cast<Roles>(role)));
}

ChatModel::Roles ChatModel::toNormalRole(const Roles role)
{
    static const QHash<Roles, Roles> convMap = {
        { Roles::RelatedMessageNickName, Roles::NickName },
        { Roles::RelatedMessageIsStateUpdate, Roles::IsStateUpdate },
        { Roles::RelatedMessageIsText, Roles::IsText },
        { Roles::RelatedMessageIsSimpleText, Roles::IsSimpleText },
        { Roles::RelatedMessageIsMultiText, Roles::IsMultiText },
        { Roles::RelatedMessageIsImage, Roles::IsImage },
        { Roles::RelatedMessageIsFile, Roles::IsFile },
        { Roles::RelatedMessageIsAudioFile, Roles::IsAudioFile },
        { Roles::RelatedMessageIsVideoFile, Roles::IsVideoFile },
        { Roles::RelatedMessageUserState, Roles::UserState },
        { Roles::RelatedMessageAffectedUserId, Roles::AffectedUserId },
        { Roles::RelatedMessageSimpleText, Roles::SimpleText },
        { Roles::RelatedMessageMultiText, Roles::MultiText },
        { Roles::RelatedMessageImageUrl, Roles::ImageUrl },
        { Roles::RelatedMessageFileUrl, Roles::FileUrl },
        { Roles::RelatedMessageFileName, Roles::FileName },
        { Roles::RelatedMessageFileSize, Roles::FileSize },
        { Roles::RelatedMessageThumbnailFileUrl, Roles::ThumbnailFileUrl },
    };

    return convMap.value(role, role);
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_chatRoom ? m_chatRoom->chatMessages().size() : 0;
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_chatRoom) {
        return QVariant();
    }

    // Find row of related message
    const auto normalizedRole = toNormalRole(role);
    if (normalizedRole != role) {
        const auto &messages = m_chatRoom->chatMessages();
        const auto item = messages.at(index.row());

        if (item->relatedMessageId().isEmpty()) {
            return QVariant();
        }

        const auto relatedMessage = m_chatRoom->chatMessageById(item->relatedMessageId());
        if (!relatedMessage) {
            return QVariant();
        }
        const auto relatedIndex = messages.indexOf(relatedMessage);
        if (relatedIndex < 0) {
            return QVariant();
        }

        return rawData(relatedIndex, normalizedRole);
    }

    return rawData(index.row(), role);
}

QVariant ChatModel::rawData(int row, int role) const
{
    const auto item = m_chatRoom->chatMessages().at(row);

    switch (role) {
    case static_cast<int>(Roles::EventId):
        return item ? item->eventId() : "";

    case static_cast<int>(Roles::RoomId):
        return m_chatRoom->id();

    case static_cast<int>(Roles::FromId):
        return item ? item->fromId() : "";

    case static_cast<int>(Roles::NickName):
        return item ? item->nickName() : "";

    case static_cast<int>(Roles::AvatarPath): {
        if (!item) {
            return "";
        }
        if (const auto user = m_chatRoom->chatUserById(item->fromId())) {
            return user->avatarPath();
        }

        return "";
    }

    case static_cast<int>(Roles::MentionedUserNames): {
        if (!item) {
            return "";
        }
        QStringList names;
        const auto users = item->mentionedUsers();
        names.reserve(users.size());

        for (const auto user : users) {
            names.append(user->computedName());
        }
        return names;
    }

    case static_cast<int>(Roles::IsStateUpdate):
        return item
                ? (qobject_cast<ChatMessageContentUserStateChange *>(item->content()) != nullptr)
                : false;

    case static_cast<int>(Roles::IsText):
        return item ? item->isText() : false;

    case static_cast<int>(Roles::IsSimpleText):
        return item ? item->isSimpleText() : false;

    case static_cast<int>(Roles::IsMultiText):
        return item ? item->isMultiText() : false;

    case static_cast<int>(Roles::IsImage):
        return item ? item->isImage() : false;

    case static_cast<int>(Roles::IsFile):
        return item ? item->isFile() : false;

    case static_cast<int>(Roles::IsAudioFile):
        return item ? item->isAudioFile() : false;

    case static_cast<int>(Roles::IsVideoFile):
        return item ? item->isVideoFile() : false;

    case static_cast<int>(Roles::UserState): {
        if (!item) {
            return QVariant::fromValue(ChatMessageContentUserStateChange::State::Unknown);
        }
        if (const auto stateContent =
                    qobject_cast<ChatMessageContentUserStateChange *>(item->content())) {
            return QVariant::fromValue(stateContent->state());
        }
        return QVariant::fromValue(ChatMessageContentUserStateChange::State::Unknown);
    }

    case static_cast<int>(Roles::AffectedUserId): {
        if (!item) {
            return "";
        }
        if (const auto stateContent =
                    qobject_cast<ChatMessageContentUserStateChange *>(item->content())) {
            return stateContent->affectedUserId();
        }
        return "";
    }

    case static_cast<int>(Roles::SimpleText): {
        if (!item) {
            return "";
        }
        if (const auto textContent = qobject_cast<ChatMessageContentText *>(item->content())) {
            const auto replacedText =
                    EmojiResolver::instance().replaceEmojiCodes(textContent->simpleText());

            if (item->flags() & ChatMessage::Flag::Markdown) {
                return highlightMentions(replacedText, *item);
            } else {
                return addLinkTags(replacedText);
            }
        }
        return "";
    }

    case static_cast<int>(Roles::MultiText): {
        if (!item) {
            return QVariant::fromValue(QList<ChatMessageContentPart *>());
        }
        if (const auto textContent = qobject_cast<ChatMessageContentText *>(item->content())) {
            return QVariant::fromValue(textContent->contentParts());
        }
        return QVariant::fromValue(QList<ChatMessageContentPart *>());
    }

    case static_cast<int>(Roles::ImageUrl): {
        if (!item) {
            return QUrl();
        }
        if (const auto imageContent = qobject_cast<ChatMessageContentImage *>(item->content())) {
            return imageContent->imagePath();
        }
        return QUrl();
    }

    case static_cast<int>(Roles::FileUrl): {
        if (!item) {
            return QUrl();
        }
        if (const auto fileContent = qobject_cast<ChatMessageContentFile *>(item->content())) {
            return fileContent->filePath();
        }
        return QUrl();
    }

    case static_cast<int>(Roles::ThumbnailFileUrl): {
        if (!item) {
            return QUrl();
        }
        if (const auto fileContent = qobject_cast<ChatMessageContentVideoFile *>(item->content())) {
            return fileContent->thumbnailFilePath();
        }
        return QUrl();
    }

    case static_cast<int>(Roles::FileName): {
        if (!item) {
            return "";
        }
        if (const auto fileContent = qobject_cast<ChatMessageContentFile *>(item->content())) {
            return fileContent->fileName();
        }
        return QString();
    }

    case static_cast<int>(Roles::FileSize): {
        if (!item) {
            return QVariant::fromValue(static_cast<qint64>(0));
        }
        if (const auto fileContent = qobject_cast<ChatMessageContentFile *>(item->content())) {
            return fileContent->fileSize();
        }
        return QVariant::fromValue(static_cast<qint64>(0));
    }

    case static_cast<int>(Roles::Timestamp):
        return item ? item->timestamp() : QVariant();

    case static_cast<int>(Roles::Reactions): {
        QVariantList l;
        if (!item) {
            return l;
        }
        const auto reactions = item->reactions();

        auto chatProvider = qobject_cast<IChatProvider *>(m_chatRoom->parent());
        QString ownUserId;
        if (chatProvider) {
            ownUserId = chatProvider->ownUserId();
        }

        for (const auto *reaction : reactions) {
            QVariantMap m;
            m.insert("reaction", reaction->reaction());
            m.insert("count", reaction->count());
            m.insert("isOwnReaction", ownUserId.isEmpty() ? false : reaction->isUser(ownUserId));
            l.append(m);
        }

        return l;
    }

    case static_cast<int>(Roles::IsPrivateMessage):
        return item ? static_cast<bool>(item->flags() & ChatMessage::Flag::PrivateMessage) : false;

    case static_cast<int>(Roles::IsOwnMessage):
        return item ? static_cast<bool>(item->flags() & ChatMessage::Flag::OwnMessage) : false;

    case static_cast<int>(Roles::IsSystemMessage):
        return item ? static_cast<bool>(item->flags() & ChatMessage::Flag::SystemMessage) : false;

    case static_cast<int>(Roles::IsEncrypted):
        return item ? static_cast<bool>(item->flags() & ChatMessage::Flag::Encrypted) : false;

    case static_cast<int>(Roles::IsPinned):
        return item ? static_cast<bool>(item->flags() & ChatMessage::Flag::Pinned) : false;

    case static_cast<int>(Roles::IsSameUserAsPrevious): {
        if (row > 0) {
            if (const auto prev = m_chatRoom->chatMessages().at(row - 1)) {
                return prev->fromId() == item->fromId();
            }
        }
        return false;
    }

    case static_cast<int>(Roles::IsSameMinuteAsPrevious): {
        if (row > 0) {
            if (const auto prev = m_chatRoom->chatMessages().at(row - 1)) {
                const auto prevDateTime = prev->timestamp();
                const auto itemDateTime = item->timestamp();

                return prevDateTime.date() == itemDateTime.date()
                        && prevDateTime.time().hour() == itemDateTime.time().hour()
                        && prevDateTime.time().minute() == itemDateTime.time().minute();
            }
        }
        return false;
    }

    case static_cast<int>(Roles::IsSameDayAsPrevious): {
        if (row > 0) {
            if (const auto prev = m_chatRoom->chatMessages().at(row - 1)) {
                return prev->timestamp().date() == item->timestamp().date();
            }
        }
        return false;
    }

    case static_cast<int>(Roles::HasRelatedMessage):
        return item ? !item->relatedMessageId().isEmpty() : false;
    }

    return QVariant();
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
                [this](qsizetype index, ChatMessage *msgObj) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                    updateRelatedMessages(msgObj->eventId(), relatedContentRoles(*msgObj));
                    updateRealMessagesCount();
                });
        connect(m_chatRoom, &IChatRoom::chatMessageRemoved, m_chatRoomContext,
                [this](qsizetype index, ChatMessage *msgObj) {
                    beginRemoveRows(QModelIndex(), index, index);
                    endRemoveRows();
                    updateRelatedMessages(msgObj->eventId(), relatedContentRoles(*msgObj));
                    updateRealMessagesCount();
                });
        connect(m_chatRoom, &IChatRoom::chatMessagesReset, m_chatRoomContext, [this]() {
            beginResetModel();
            endResetModel();
            updateRealMessagesCount();
        });
        connect(m_chatRoom, &IChatRoom::chatMessageContentChanged, m_chatRoomContext,
                [this](qsizetype idx, ChatMessage *msgObj) {
                    const auto modelIndex = createIndex(idx, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex,
                                       { static_cast<int>(Roles::IsSimpleText),
                                         static_cast<int>(Roles::IsMultiText),
                                         static_cast<int>(Roles::SimpleText),
                                         static_cast<int>(Roles::MultiText),
                                         static_cast<int>(Roles::ThumbnailFileUrl) });

                    updateRelatedMessages(msgObj->eventId(), relatedContentRoles(*msgObj));
                });
        connect(m_chatRoom, &IChatRoom::chatMessageMentionedUsersChanged, m_chatRoomContext,
                [this](qsizetype idx, ChatMessage *) {
                    const auto modelIndex = createIndex(idx, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex,
                                       { static_cast<int>(Roles::MentionedUserNames) });
                });
        connect(m_chatRoom, &IChatRoom::chatMessageFlagsChanged, m_chatRoomContext,
                [this](qsizetype idx, ChatMessage *) {
                    const auto modelIndex = createIndex(idx, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex,
                                       { static_cast<int>(Roles::IsEncrypted),
                                         static_cast<int>(Roles::IsPinned) });
                });
        connect(m_chatRoom, &IChatRoom::chatMessageReactionsChanged, m_chatRoomContext,
                [this](qsizetype idx, ChatMessage *) {
                    const auto modelIndex = createIndex(idx, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex,
                                       { static_cast<int>(Roles::Reactions) });
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
        Q_EMIT realMessagesCountChanged();
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

QString ChatModel::highlightMentions(const QString &orig, const ChatMessage &message) const
{
    const auto mentions = message.mentionedUsers();
    if (mentions.isEmpty()) {
        return orig;
    }

    QString str(orig);

    for (const auto *user : mentions) {
        const auto name = user->computedName();
        if (name.isEmpty()) {
            continue;
        }

        // Use word boundaries (\b) to prevent name splitting.
        // escape(name) prevents regex injections.
        const QRegularExpression regex(QString(R"(\b%1\b)").arg(QRegularExpression::escape(name)));
        const QString replacement = QString("[%1](chat://%2)").arg(name, user->id());
        str.replace(regex, replacement);
    }

    return str;
}

ChatMessage *ChatModel::relatedMessage(ChatMessage *originalMessage) const
{
    if (originalMessage && !originalMessage->relatedMessageId().isEmpty()) {
        return m_chatRoom->chatMessageById(originalMessage->relatedMessageId());
    }

    return nullptr;
}

void ChatModel::updateRelatedMessages(const QString &originalMessageId, const QList<int> &roles)
{
    if (originalMessageId.isEmpty() || roles.isEmpty()) {
        return;
    }

    const auto messages = m_chatRoom->chatMessages();
    const qsizetype l = messages.length();

    for (qsizetype i = 0; i < l; ++i) {
        const auto chatMsg = messages.at(i);
        if (chatMsg->relatedMessageId() == originalMessageId) {
            const auto modelIndex = createIndex(i, 0);
            Q_EMIT dataChanged(modelIndex, modelIndex, roles);
        }
    }
}

QList<int> ChatModel::relatedContentRoles(const ChatMessage &messageObject) const
{
    QList<int> roles = { static_cast<int>(Roles::RelatedMessageNickName) };
    const auto content = messageObject.content();

    if (qobject_cast<ChatMessageContentUserStateChange *>(content)) {
        roles.append(static_cast<int>(Roles::RelatedMessageUserState));
        roles.append(static_cast<int>(Roles::RelatedMessageAffectedUserId));

    } else if (qobject_cast<ChatMessageContentText *>(content)) {
        roles.append(static_cast<int>(Roles::RelatedMessageSimpleText));
        roles.append(static_cast<int>(Roles::RelatedMessageMultiText));
        roles.append(static_cast<int>(Roles::RelatedMessageIsMultiText));
        roles.append(static_cast<int>(Roles::RelatedMessageIsSimpleText));

    } else if (qobject_cast<ChatMessageContentImage *>(content)) {
        roles.append(static_cast<int>(Roles::RelatedMessageImageUrl));

    } else if (qobject_cast<ChatMessageContentFile *>(content)) {
        roles.append(static_cast<int>(Roles::RelatedMessageFileUrl));
        roles.append(static_cast<int>(Roles::RelatedMessageFileName));
        roles.append(static_cast<int>(Roles::RelatedMessageFileSize));
        roles.append(static_cast<int>(Roles::RelatedMessageThumbnailFileUrl));
    }

    return roles;
}
