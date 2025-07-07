#include "ChatRoomProxyModel.h"
#include "ChatRoomModel.h"

ChatRoomProxyModel::ChatRoomProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    sort(0);
}

bool ChatRoomProxyModel::lessThan(const QModelIndex &sourceLeft,
                                    const QModelIndex &sourceRight) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    using Roles = ChatRoomModel::Roles;

    const auto leftName = model->data(sourceLeft, static_cast<int>(Roles::Name)).toString();
    const auto rightName = model->data(sourceRight, static_cast<int>(Roles::Name)).toString();

    return leftName.localeAwareCompare(rightName) < 0;
}
