#include "UsersModel.h"
#include "IConferenceConnector.h"
#include "ConferenceUser.h"

UsersModel::UsersModel(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &UsersModel::conferenceConnectorChanged, this,
            &UsersModel::onConferenceConnectorChanged);
}

QHash<int, QByteArray> UsersModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::DisplayName), "displayName" },
        { static_cast<int>(Roles::Role), "role" },
        { static_cast<int>(Roles::IsAudioMuted), "isAudioMuted" },
        { static_cast<int>(Roles::IsVideoMuted), "isVideoMuted" },
        { static_cast<int>(Roles::IsHandRaised), "isHandRaised" },
        { static_cast<int>(Roles::IsDominantSpeaker), "isDominantSpeaker" },
    };
}

int UsersModel::rowCount(const QModelIndex &) const
{
    return m_conferenceConnector ? m_conferenceConnector->users().size() : 0;
}

QVariant UsersModel::data(const QModelIndex &index, int role) const
{
    if (!m_conferenceConnector || !index.isValid()) {
        return QVariant();
    }

    const auto user = m_conferenceConnector->users().at(index.row());

    switch (static_cast<Roles>(role)) {
    case Roles::Id:
        return user->id();
    case Roles::Role:
        return static_cast<int>(user->role());
    case Roles::IsAudioMuted:
        return user->isAudioMuted();
    case Roles::IsVideoMuted:
        return user->isVideoMuted();
    case Roles::IsHandRaised:
        return user->isHandRaised();
    case Roles::IsDominantSpeaker:
        return user->isDominantSpeaker();

    default:
    case Roles::DisplayName:
        return user->displayName();
    }
}

void UsersModel::onConferenceConnectorChanged()
{
    beginResetModel();

    if (m_jistiConnectorContext) {
        m_jistiConnectorContext->deleteLater();
        m_jistiConnectorContext = nullptr;
    }

    if (m_conferenceConnector) {
        m_jistiConnectorContext = new QObject(this);

        connect(m_conferenceConnector, &IConferenceConnector::usersCleared, this, [this]() {
            beginResetModel();
            endResetModel();
        });

        connect(m_conferenceConnector, &IConferenceConnector::userAdded, this,
                [this](qsizetype index, ConferenceUser *) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                });

        connect(m_conferenceConnector, &IConferenceConnector::userRemoved, this,
                [this](qsizetype index, ConferenceUser *) {
                    beginRemoveRows(QModelIndex(), index, index);
                    endRemoveRows();
                });

        connect(m_conferenceConnector, &IConferenceConnector::userRoleChanged, this,
                [this](qsizetype index, ConferenceUser *, ConferenceUser::Role) {
                    const auto modelIndex = createIndex(index, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex, { static_cast<int>(Roles::Role) });
                });

        connect(m_conferenceConnector, &IConferenceConnector::userChanged, this,
                [this](qsizetype index, ConferenceUser *) {
                    const auto modelIndex = createIndex(index, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex);
                });
    }

    endResetModel();
}
