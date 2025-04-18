#include "HistoryProxyModel.h"
#include "HistoryModel.h"
#include "CallHistoryItem.h"

HistoryProxyModel::HistoryProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &HistoryProxyModel::filterTextChanged, this, [this]() { invalidateFilter(); });
    connect(this, &HistoryProxyModel::typeFilterChanged, this, [this]() { invalidateFilter(); });
}

bool HistoryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);
    const auto type = static_cast<CallHistoryItem::Type>(
            model->data(index, static_cast<int>(HistoryModel::Roles::Type)).toInt());
    const bool wasEstablished =
            model->data(index, static_cast<int>(HistoryModel::Roles::WasEstablished)).toBool();

    if (m_typeFilter == TypeFilter::INCOMING && type != CallHistoryItem::Type::Incoming
        && type != CallHistoryItem::Type::IncomingBlocked) {
        return false;
    }
    if (m_typeFilter == TypeFilter::OUTGOING && type != CallHistoryItem::Type::Outgoing) {
        return false;
    }
    if (m_typeFilter == TypeFilter::MISSED
        && (wasEstablished || type == CallHistoryItem::Type::Outgoing)) {
        return false;
    }

    const auto remoteUrl =
            model->data(index, static_cast<int>(HistoryModel::Roles::RemoteUrl)).toString();
    if (remoteUrl.contains(m_filterText, Qt::CaseInsensitive)) {
        return true;
    }

    const auto contactName =
            model->data(index, static_cast<int>(HistoryModel::Roles::ContactName)).toString();
    return contactName.contains(m_filterText, Qt::CaseInsensitive);
}
