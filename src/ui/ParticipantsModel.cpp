#include "ParticipantsModel.h"
#include "JitsiConnector.h"

ParticipantsModel::ParticipantsModel(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &ParticipantsModel::jitsiConnectorChanged, this,
            &ParticipantsModel::onJitsiConnectorChanged);
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
    return m_jitsiConnector ? m_jitsiConnector->participants().size() : 0;
}

QVariant ParticipantsModel::data(const QModelIndex &index, int role) const
{
    if (!m_jitsiConnector || !index.isValid()) {
        return QVariant();
    }

    const auto &participant = m_jitsiConnector->participants().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Id):
        return participant.id;

    case static_cast<int>(Roles::Role):
        return static_cast<int>(participant.role);

    case static_cast<int>(Roles::DisplayName):
    default:
        return participant.displayName;
    }
}

void ParticipantsModel::onJitsiConnectorChanged()
{
    beginResetModel();

    if (m_jistiConnectorContext) {
        m_jistiConnectorContext->deleteLater();
        m_jistiConnectorContext = nullptr;
    }

    if (m_jitsiConnector) {
        m_jistiConnectorContext = new QObject(this);

        connect(m_jitsiConnector, &JitsiConnector::participantsCleared, this, [this]() {
            beginResetModel();
            endResetModel();
        });

        connect(m_jitsiConnector, &JitsiConnector::participantAdded, this,
                [this](qsizetype index, const QString) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                });

        connect(m_jitsiConnector, &JitsiConnector::participantRemoved, this,
                [this](qsizetype index, const QString) {
                    beginRemoveRows(QModelIndex(), index, index);
                    endRemoveRows();
                });

        connect(m_jitsiConnector, &JitsiConnector::participantRoleChanged, this,
                [this](qsizetype index, const QString, const JitsiConnector::ParticipantRole) {
                    const auto modelIndex = createIndex(index, 0);
                    emit dataChanged(modelIndex, modelIndex, { static_cast<int>(Roles::Role) });
                });
    }

    endResetModel();
}
