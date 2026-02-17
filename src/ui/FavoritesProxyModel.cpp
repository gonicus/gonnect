#include "FavoritesProxyModel.h"
#include "FavoritesModel.h"
#include "NumberStats.h"

FavoritesProxyModel::FavoritesProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{

    connect(this, &FavoritesProxyModel::showJitsiChanged, this, [this]() {
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

    if (!m_showJitsi
        && qvariant_cast<ContactType>(model->data(index, static_cast<int>(Roles::ContactType)))
                == ContactType::JitsiMeetUrl) {
        return false;
    }

    return true;
}
