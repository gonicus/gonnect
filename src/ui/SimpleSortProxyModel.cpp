#include "SimpleSortProxyModel.h"

SimpleSortProxyModel::SimpleSortProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &SimpleSortProxyModel::sortRoleNameChanged, this,
            &SimpleSortProxyModel::updateSorting);
    connect(this, &SimpleSortProxyModel::sourceModelChanged, this,
            &SimpleSortProxyModel::updateSorting);

    setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    sort(0, Qt::AscendingOrder);
}

void SimpleSortProxyModel::updateSorting()
{
    int newRole = Qt::DisplayRole;
    const auto model = sourceModel();

    if (model && !m_sortRoleName.isEmpty()) {
        const auto roleNames = model->roleNames();

        QHashIterator it(roleNames);
        while (it.hasNext()) {
            it.next();
            if (it.value() == m_sortRoleName) {
                newRole = it.key();
                break;
            }
        }
    }

    setSortRole(newRole);
}
