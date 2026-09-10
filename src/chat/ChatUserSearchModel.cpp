#include "ChatUserSearchModel.h"
#include "ChatUser.h"
#include "AddressBook.h"
#include "AvatarPrioHelper.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcChatUserSearchModel, "gonnect.app.chat.ChatUserSearchModel")

ChatUserSearchModel::ChatUserSearchModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatUserSearchModel::chatProviderChanged, this,
            &ChatUserSearchModel::onChatProviderChanged);

    connect(this, &ChatUserSearchModel::searchPhraseChanged, this, [this]() {
        if (!m_chatProvider) {
            qCWarning(lcChatUserSearchModel)
                    << "Cannot search for users as there is no chat provider set";
            return;
        }

        m_searchId = m_chatProvider->searchChatUser(m_searchPhrase.trimmed());

        if (m_searchId.isEmpty()) {
            qCWarning(lcChatUserSearchModel) << "id of searchChatUser is not set";
        }
    });
}

QHash<int, QByteArray> ChatUserSearchModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::HasPresenceState), "hasPresenceState" },
        { static_cast<int>(Roles::PresenceState), "presenceState" },
    };
}

int ChatUserSearchModel::rowCount(const QModelIndex &) const
{
    return m_model.size();
}

QVariant ChatUserSearchModel::data(const QModelIndex &index, int role) const
{
    const auto user = std::as_const(m_model).at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Id):
        return user->id();

    case static_cast<int>(Roles::AvatarPath): {
        if (const auto *contact = AddressBook::instance().lookupByChatUser(user)) {
            return contact->avatarPath();
        }
        return user->avatarPath();
    }

    case static_cast<int>(Roles::HasPresenceState):
        return user->hasPresenceState();

    case static_cast<int>(Roles::PresenceState):
        return QVariant::fromValue(user->presenceState());

    case static_cast<int>(Roles::Name):
    default:
        return user->computedName();
    }
}

void ChatUserSearchModel::onChatProviderChanged()
{
    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }
    m_avatarSignaledUsers.clear();

    if (m_chatProvider) {
        m_chatProviderContext = new QObject(this);
        connect(&AddressBook::instance(), &AddressBook::chatUserMappingAdded, m_chatProviderContext,
                [this](ChatUser *user) { refreshAvatarPath(user); });
        connect(&AddressBook::instance(), &AddressBook::chatUserAvatarChanged,
                m_chatProviderContext, [this](ChatUser *user) { refreshAvatarPath(user); });
        connect(&AvatarPrioHelper::instance(), &AvatarPrioHelper::priosChanged,
                m_chatProviderContext, [this]() {
                    const auto rows = rowCount(QModelIndex());
                    if (rows > 0) {
                        Q_EMIT dataChanged(createIndex(0, 0), createIndex(rows - 1, 0),
                                           { static_cast<int>(Roles::AvatarPath) });
                    }
                });

        connect(m_chatProvider, &IChatProvider::userRemoved, m_chatProviderContext,
                [this](QString, ChatUser *user, qsizetype) {
                    const auto idx = m_model.indexOf(user);
                    if (idx >= 0) {
                        beginRemoveRows(QModelIndex(), idx, idx);
                        m_model.removeAt(idx);
                        endRemoveRows();
                    }
                });

        connect(m_chatProvider, &IChatProvider::chatUserSearchResult, m_chatProviderContext,
                [this](QString id, QList<ChatUser *> userList) {
                    if (m_searchId == id) {
                        updateModel(userList);
                    }
                });
    }

    updateModel({});
}

void ChatUserSearchModel::updateModel(const QList<ChatUser *> &userList)
{
    if (m_model != userList) {
        beginResetModel();
        m_model = userList;
        endResetModel();

        for (auto *user : userList) {
            connectUserAvatarSignals(user);
        }
    }
}

void ChatUserSearchModel::connectUserAvatarSignals(ChatUser *user)
{
    if (m_avatarSignaledUsers.contains(user)) {
        return;
    }
    m_avatarSignaledUsers.insert(user);

    connect(user, &ChatUser::avatarPathChanged, m_chatProviderContext,
            [this, user]() { refreshAvatarPath(user); });
}

void ChatUserSearchModel::refreshAvatarPath(ChatUser *user)
{
    const auto row = m_model.indexOf(user);
    if (row < 0) {
        return;
    }
    const auto idx = createIndex(row, 0);
    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::AvatarPath) });
}
