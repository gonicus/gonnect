#include "ParticipantsModel.h"
#include "IConferenceConnector.h"
#include "ConferenceParticipant.h"

ParticipantsModel::ParticipantsModel(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &ParticipantsModel::iConferenceConnectorChanged, this,
            &ParticipantsModel::onIConferenceConnectorChanged);
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
    return m_iConferenceConnector ? m_iConferenceConnector->participants().size() : 0;
}

QVariant ParticipantsModel::data(const QModelIndex &index, int role) const
{
    if (!m_iConferenceConnector || !index.isValid()) {
        return QVariant();
    }

    const auto participant = m_iConferenceConnector->participants().at(index.row());

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

void ParticipantsModel::onIConferenceConnectorChanged()
{
    beginResetModel();

    if (m_jistiConnectorContext) {
        m_jistiConnectorContext->deleteLater();
        m_jistiConnectorContext = nullptr;
    }

    if (m_iConferenceConnector) {
        m_jistiConnectorContext = new QObject(this);

        connect(m_iConferenceConnector, &IConferenceConnector::participantsCleared, this, [this]() {
            beginResetModel();
            endResetModel();
        });

        connect(m_iConferenceConnector, &IConferenceConnector::participantAdded, this,
                [this](qsizetype index, ConferenceParticipant *) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                });

        connect(m_iConferenceConnector, &IConferenceConnector::participantRemoved, this,
                [this](qsizetype index, ConferenceParticipant *) {
                    beginRemoveRows(QModelIndex(), index, index);
                    endRemoveRows();
                });

        connect(m_iConferenceConnector, &IConferenceConnector::participantRoleChanged, this,
                [this](qsizetype index, ConferenceParticipant *, ConferenceParticipant::Role) {
                    const auto modelIndex = createIndex(index, 0);
                    emit dataChanged(modelIndex, modelIndex, { static_cast<int>(Roles::Role) });
                });
    }

    endResetModel();
}
