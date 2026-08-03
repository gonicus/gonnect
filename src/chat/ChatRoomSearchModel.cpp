#include "ChatRoomSearchModel.h"
#include "IChatProvider.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcChatSearchModel, "gonnect.app.chat.ChatRoomSearchModel")

ChatRoomSearchModel::ChatRoomSearchModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatRoomSearchModel::chatProviderChanged, this,
            &ChatRoomSearchModel::onChatProviderChanged);
    connect(this, &ChatRoomSearchModel::searchPhraseChanged, this,
            &ChatRoomSearchModel::updateModel);
    updateModel();
}

QHash<int, QByteArray> ChatRoomSearchModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::Topic), "topic" },
        { static_cast<int>(Roles::NumberOfJoinedMembers), "numberOfJoinedMembers" },
        { static_cast<int>(Roles::JoinRule), "joinRule" },
    };
}

int ChatRoomSearchModel::rowCount(const QModelIndex &) const
{
    return m_publicRooms.length();
}

QVariant ChatRoomSearchModel::data(const QModelIndex &index, int role) const
{
    const auto roomPtr = m_publicRooms.at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Id):
        return roomPtr->roomId;

    case static_cast<int>(Roles::Topic):
        return roomPtr->topic;

    case static_cast<int>(Roles::NumberOfJoinedMembers):
        return roomPtr->numberOfJoinedMembers;

    case static_cast<int>(Roles::JoinRule):
        return QVariant::fromValue(roomPtr->joinRule);

    case static_cast<int>(Roles::Name):
    default:
        return roomPtr->displayName;
    }
}

void ChatRoomSearchModel::loadNext()
{
    if (!m_chatProvider) {
        qCWarning(lcChatSearchModel) << "Cannot load results without chat provider";
        return;
    }
    if (m_nextBatchToken.isEmpty()) {
        qCInfo(lcChatSearchModel) << "Have no next batch token - not requesting new results";
        return;
    }

    setCanLoadMore(false);
    m_isLoadingNext = true;
    setIsLoading(true);

    m_searchTag =
            m_chatProvider->searchPublicRoomRequest(m_searchPhrase, m_nextBatchToken, m_limit);
}

void ChatRoomSearchModel::onChatProviderChanged()
{
    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }

    if (m_chatProvider) {
        m_chatProviderContext = new QObject(this);

        connect(m_chatProvider, &IChatProvider::publicRoomSearchResult, m_chatProviderContext,
                [this](QString searchId, QList<QSharedPointer<PublicChatRoom>> roomList,
                       QString nextBatchToken) {
                    if (m_searchTag == searchId) {
                        m_searchTag.clear();
                    }
                    m_nextBatchToken = nextBatchToken;
                    setCanLoadMore(!m_nextBatchToken.isEmpty());

                    if (m_isLoadingNext) {
                        m_isLoadingNext = false;
                        const auto l = m_publicRooms.length();
                        beginInsertRows(QModelIndex(), l, l + roomList.length());
                        m_publicRooms += roomList;
                        endInsertRows();
                    } else {
                        beginResetModel();
                        m_publicRooms = roomList;
                        endResetModel();
                    }

                    setIsLoading(false);
                });

        updateModel();
    }
}

void ChatRoomSearchModel::updateModel()
{
    const auto searchPhrase = m_searchPhrase.trimmed();

    setCanLoadMore(false);
    m_searchTag = QString();
    m_nextBatchToken = QString();
    m_isLoadingNext = false;

    if (m_chatProvider && (m_limit > 0 || !searchPhrase.isEmpty())) {
        setIsLoading(true);
        m_searchTag =
                m_chatProvider->searchPublicRoomRequest(searchPhrase, m_nextBatchToken, m_limit);
    }
}

void ChatRoomSearchModel::setIsLoading(bool value)
{
    if (m_isLoading != value) {
        m_isLoading = value;
        Q_EMIT isLoadingChanged();
    }
}

void ChatRoomSearchModel::setCanLoadMore(bool value)
{
    if (m_canLoadMore != value) {
        m_canLoadMore = value;
        Q_EMIT canLoadMoreChanged();
    }
}
