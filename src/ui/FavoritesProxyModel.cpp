#include "FavoritesProxyModel.h"
#include "FavoritesModel.h"
#include "NumberStats.h"

FavoritesProxyModel::FavoritesProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    sort(0);

    connect(this, &FavoritesProxyModel::showJitsiChanged, this, [this]() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        endFilterChange();
#else
        invalidateRowsFilter();
#endif
    });
    connect(this, &FavoritesProxyModel::showChatRoomsChanged, this, [this]() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        endFilterChange();
#else
        invalidateRowsFilter();
#endif
    });
}

bool FavoritesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    using Roles = FavoritesModel::Roles;
    using ContactType = NumberStats::ContactType;

    const auto model = qobject_cast<FavoritesModel *>(sourceModel());
    if (!model) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);
    const auto contactType =
            qvariant_cast<ContactType>(model->data(index, static_cast<int>(Roles::ContactType)));

    if (!m_showJitsi && contactType == ContactType::JitsiMeetUrl) {
        return false;
    }
    if (!m_showChatRooms && contactType == ContactType::ChatRoomId) {
        return false;
    }

    return true;
}

bool FavoritesProxyModel::lessThan(const QModelIndex &sourceLeft,
                                   const QModelIndex &sourceRight) const
{
    using Roles = FavoritesModel::Roles;

    const auto model = qobject_cast<FavoritesModel *>(sourceModel());
    if (!model) {
        return false;
    }

    const auto leftName = model->data(sourceLeft, static_cast<int>(Roles::Name)).toString();
    const auto rightName = model->data(sourceRight, static_cast<int>(Roles::Name)).toString();

    return leftName.localeAwareCompare(rightName) < 0;
}
