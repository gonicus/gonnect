#include "ParticipantsModel.h"
#include "IConferenceConnector.h"
#include "ConferenceParticipant.h"

ParticipantsModel::ParticipantsModel(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &ParticipantsModel::conferenceConnectorChanged, this,
            &ParticipantsModel::onConferenceConnectorChanged);
}

QHash<int, QByteArray> ParticipantsModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::DisplayName), "displayName" },
        { static_cast<int>(Roles::Role), "role" },
    };
}

int ParticipantsModel::rowCount(const QModelIndex &) const
{
    return m_conferenceConnector ? m_conferenceConnector->participants().size() : 0;
}

QVariant ParticipantsModel::data(const QModelIndex &index, int role) const
{
    if (!m_conferenceConnector || !index.isValid()) {
        return QVariant();
    }

    const auto participant = m_conferenceConnector->participants().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Id):
        return participant->id();

    case static_cast<int>(Roles::Role):
        return static_cast<int>(participant->role());

    case static_cast<int>(Roles::DisplayName):
    default:
        return participant->displayName();
    }
}

void ParticipantsModel::onConferenceConnectorChanged()
{
    beginResetModel();

    if (m_jistiConnectorContext) {
        m_jistiConnectorContext->deleteLater();
        m_jistiConnectorContext = nullptr;
    }

    if (m_conferenceConnector) {
        m_jistiConnectorContext = new QObject(this);

        connect(m_conferenceConnector, &IConferenceConnector::participantsCleared, this, [this]() {
            beginResetModel();
            endResetModel();
        });

        connect(m_conferenceConnector, &IConferenceConnector::participantAdded, this,
                [this](qsizetype index, ConferenceParticipant *) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                });

        connect(m_conferenceConnector, &IConferenceConnector::participantRemoved, this,
                [this](qsizetype index, ConferenceParticipant *) {
                    beginRemoveRows(QModelIndex(), index, index);
                    endRemoveRows();
                });

        connect(m_conferenceConnector, &IConferenceConnector::participantRoleChanged, this,
                [this](qsizetype index, ConferenceParticipant *, ConferenceParticipant::Role) {
                    const auto modelIndex = createIndex(index, 0);
                    emit dataChanged(modelIndex, modelIndex, { static_cast<int>(Roles::Role) });
                });
    }

    endResetModel();
}
