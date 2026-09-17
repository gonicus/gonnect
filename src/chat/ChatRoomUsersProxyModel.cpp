#include "ChatRoomUsersProxyModel.h"
#include "ChatRoomUsers.h"

ChatRoomUsersProxyModel::ChatRoomUsersProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &ChatRoomUsersProxyModel::filterTextChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });
    connect(this, &ChatRoomUsersProxyModel::excludedUserIdsChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });

    sort(0);
}

bool ChatRoomUsersProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    if (m_filterText.isEmpty()) {
        return true;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);
    using Roles = ChatRoomUsers::Roles;

    const auto id = model->data(index, static_cast<int>(Roles::Id)).toString();
    if (m_excludedUserIds.contains(id)) {
        return false;
    }
    if (id.contains(m_filterText, Qt::CaseInsensitive)) {
        return true;
    }

    const auto computedName = model->data(index, static_cast<int>(Roles::ComputedName)).toString();
    if (computedName.contains(m_filterText, Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}

bool ChatRoomUsersProxyModel::lessThan(const QModelIndex &sourceLeft,
                                       const QModelIndex &sourceRight) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    using Roles = ChatRoomUsers::Roles;
    const auto leftName = model->data(sourceLeft, static_cast<int>(Roles::ComputedName)).toString();
    const auto rightName =
            model->data(sourceRight, static_cast<int>(Roles::ComputedName)).toString();

    return leftName.localeAwareCompare(rightName) < 0;
}
