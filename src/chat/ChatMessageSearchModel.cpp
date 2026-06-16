#include <QObject>

#include "ChatMessageSearchModel.h"

ChatMessageSearchModel::ChatMessageSearchModel(QObject *parent) : QAbstractListModel{ parent } { }

QHash<int, QByteArray> ChatMessageSearchModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::MessageUid), "messageUid" },
        { static_cast<int>(Roles::Rank), "rank" },
    };
}

int ChatMessageSearchModel::rowCount(const QModelIndex &) const
{
    return m_results.size();
}

QVariant ChatMessageSearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_results.size()) {
        return QVariant();
    }

    const auto result = m_results.at(index.row());

    switch (role) {
    case static_cast<int>(Roles::MessageUid):
        return result.messageUid;
    case static_cast<int>(Roles::Rank):
        return result.rank;
    default:
        return result.messageUid;
    }
}

void ChatMessageSearchModel::addResults(
        const QList<ChatMessageSearchIndexer::SearchResult> &results)
{
    if (results.isEmpty()) {
        return;
    }

    int firstRow = m_results.size();
    int lastRow = firstRow + results.size() - 1;

    beginInsertRows(QModelIndex(), firstRow, lastRow);
    for (auto &result : results) {
        m_results.append(result);
    }
    endInsertRows();
}

void ChatMessageSearchModel::reset()
{
    beginResetModel();
    m_results.clear();
    endResetModel();
}
