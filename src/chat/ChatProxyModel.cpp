#include "ChatProxyModel.h"
#include "ChatModel.h"

ChatProxyModel::ChatProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &ChatProxyModel::threadIdChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });

    sort(0);
}

bool ChatProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    using Roles = ChatModel::Roles;

    const auto leftTime = model->data(sourceLeft, static_cast<int>(Roles::Timestamp)).toDateTime();
    const auto rightTime =
            model->data(sourceRight, static_cast<int>(Roles::Timestamp)).toDateTime();

    return leftTime > rightTime;
}

bool ChatProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    if (m_threadId.isEmpty()) {
        return true;
    }

    using Roles = ChatModel::Roles;
    const auto sourceIndex = model->index(sourceRow, 0, sourceParent);
    if (!sourceIndex.isValid()) {
        return false;
    }

    const auto threadId = model->data(sourceIndex, static_cast<int>(Roles::ThreadId)).toString();
    return m_threadId == threadId;
}
