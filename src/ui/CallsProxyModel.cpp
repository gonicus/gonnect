#include "CallsProxyModel.h"
#include "CallsModel.h"

CallsProxyModel::CallsProxyModel(QObject *parent) : QSortFilterProxyModel{ parent } { }

bool CallsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);

    return !model->data(index, static_cast<int>(CallsModel::Roles::IsBlocked)).toBool();
}
