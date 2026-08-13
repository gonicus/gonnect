#include "RTTModel.h"

RTTModel::RTTModel(QObject *parent) : QAbstractListModel(parent) { }

int RTTModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_messages.size();
}

QVariant RTTModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.size()) {
        return QVariant();
    }

    const RTTMessage &msg = m_messages.at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Timestamp):
        return msg.timestamp();
    case static_cast<int>(Roles::Message):
        return msg.message();
    case static_cast<int>(Roles::IsMe):
        return msg.isMe();
    case static_cast<int>(Roles::IsFinished):
        return msg.isFinished();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> RTTModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Timestamp), "timestamp" },
        { static_cast<int>(Roles::Message), "message" },
        { static_cast<int>(Roles::IsMe), "isMe" },
        { static_cast<int>(Roles::IsFinished), "isFinished" },
    };
}

void RTTModel::reset()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

void RTTModel::addMessage(qint64 timestamp, const QString &message, bool isMe)
{
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(RTTMessage{ timestamp, message, isMe, false });
    endInsertRows();
}

void RTTModel::updateMessage(const QString &message, bool isMe, bool isFinished)
{
    if (m_messages.isEmpty()) {
        return;
    }

    // Retrieve the last unfinished message of the participant
    int targetRow = -1;
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        auto &msg = m_messages[i];
        if (msg.isMe() == isMe && !msg.isFinished()) {
            targetRow = i;
            break;
        }
    }

    if (targetRow == -1) {
        return;
    }

    m_messages[targetRow].setMessage(message);
    m_messages[targetRow].setIsFinished(isFinished);

    QModelIndex idx = index(targetRow);
    Q_EMIT dataChanged(idx, idx,
                       { static_cast<int>(Roles::Message), static_cast<int>(Roles::IsFinished) });
}
