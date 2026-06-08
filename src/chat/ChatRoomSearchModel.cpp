#include "ChatRoomSearchModel.h"
#include "IChatProvider.h"

ChatRoomSearchModel::ChatRoomSearchModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatRoomSearchModel::chatProviderChanged, this,
            &ChatRoomSearchModel::onChatProviderChanged);
    connect(this, &ChatRoomSearchModel::searchPhraseChanged, this, [this]() {
        if (m_chatProvider) {
            m_lastSearchId = m_chatProvider->searchPublicRoomRequest(m_searchPhrase);
        }
    });
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

void ChatRoomSearchModel::onChatProviderChanged()
{
    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }

    if (m_chatProvider) {
        m_chatProviderContext = new QObject(this);

        connect(m_chatProvider, &IChatProvider::publicRoomSearchResult, m_chatProviderContext,
                [this](QString searchId, QList<QSharedPointer<PublicChatRoom>> roomList, QString) {
                    if (m_lastSearchId == searchId) {
                        m_lastSearchId.clear();
                    }

                    beginResetModel();
                    m_publicRooms = roomList;
                    endResetModel();
                });
    }
}
