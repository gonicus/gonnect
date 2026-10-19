#include "TranscriptionChunksModel.h"

TranscriptionChunksModel::TranscriptionChunksModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &TranscriptionChunksModel::conferenceConnectorChanged, this,
            &TranscriptionChunksModel::onConnectorChanged);
}

QHash<int, QByteArray> TranscriptionChunksModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::MessageId), "messageId" },
        { static_cast<int>(Roles::ParticipantId), "participantId" },
        { static_cast<int>(Roles::ParticipantName), "participantName" },
        { static_cast<int>(Roles::ParticipantAvatarUrl), "participantAvatarUrl" },
        { static_cast<int>(Roles::Language), "language" },
        { static_cast<int>(Roles::FinalText), "finalText" },
        { static_cast<int>(Roles::StableText), "stableText" },
        { static_cast<int>(Roles::UnstableText), "unstableText" },
        { static_cast<int>(Roles::ReceivedAt), "receivedAt" },
    };
}

int TranscriptionChunksModel::rowCount(const QModelIndex &) const
{
    return m_connector ? m_connector->transcriptionChunks().size() : 0;
}

QVariant TranscriptionChunksModel::data(const QModelIndex &index, int role) const
{
    if (!m_connector || !index.isValid()) {
        return QVariant();
    }

    const auto &chunk = m_connector->transcriptionChunks().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::MessageId):
        return chunk.messageId;
    case static_cast<int>(Roles::ParticipantId):
        return chunk.participantId;
    case static_cast<int>(Roles::ParticipantName):
        return chunk.participantName;
    case static_cast<int>(Roles::ParticipantAvatarUrl):
        return chunk.participantAvatarUrl;
    case static_cast<int>(Roles::Language):
        return chunk.language;
    case static_cast<int>(Roles::FinalText):
        return chunk.finalText;
    case static_cast<int>(Roles::StableText):
        return chunk.stableText;
    case static_cast<int>(Roles::UnstableText):
        return chunk.unstableText;
    case static_cast<int>(Roles::ReceivedAt):
        return chunk.receivedAt;
    }

    return QVariant();
}

void TranscriptionChunksModel::onConnectorChanged()
{
    if (m_connectorContext) {
        delete m_connectorContext;
        m_connectorContext = nullptr;
    }

    if (m_connector) {
        m_connectorContext = new QObject(this);

        connect(m_connector, &QObject::destroyed, m_connectorContext, [this]() {
            m_connector = nullptr;
            onConnectorChanged();
        });

        connect(m_connector, &IConferenceConnector::transcriptionChunkAdded, m_connectorContext,
                [this](qsizetype index) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                });

        connect(m_connector, &IConferenceConnector::transcriptionChunkUpdated, m_connectorContext,
                [this](qsizetype index) {
                    const auto idx = this->index(index, 0);
                    Q_EMIT dataChanged(idx, idx);
                });
    }
}
