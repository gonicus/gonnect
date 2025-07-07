#include "ChatProxyModel.h"
#include "ChatModel.h"

ChatProxyModel::ChatProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
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

    return leftTime < rightTime;
}
