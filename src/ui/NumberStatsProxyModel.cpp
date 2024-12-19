#include "NumberStatsProxyModel.h"
#include "NumberStatsModel.h"

NumberStatsProxyModel::NumberStatsProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    sort(0);
}

bool NumberStatsProxyModel::lessThan(const QModelIndex &sourceLeft,
                                     const QModelIndex &sourceRight) const
{
    const auto model = sourceModel();

    if (!model) {
        return false;
    }

    typedef NumberStatsModel::Roles Roles;

    const auto leftCount = model->data(sourceLeft, static_cast<int>(Roles::Count)).toUInt();
    const auto rightCount = model->data(sourceRight, static_cast<int>(Roles::Count)).toUInt();

    if (leftCount == rightCount) {
        const auto leftNumber =
                model->data(sourceLeft, static_cast<int>(Roles::PhoneNumber)).toString();
        const auto rightNumber =
                model->data(sourceRight, static_cast<int>(Roles::PhoneNumber)).toString();
        return leftNumber < rightNumber;
    }

    return leftCount > rightCount;
}
