#include "TogglerProxyModel.h"
#include "TogglerModel.h"

TogglerProxyModel::TogglerProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &TogglerProxyModel::displayFilterChanged, this, [this]() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        endFilterChange();
#else
        invalidateRowsFilter();
#endif
    });
}

bool TogglerProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    if (m_displayFilter) {
        const auto index = model->index(sourceRow, 0, sourceParent);
        const auto display =
                model->data(index, static_cast<int>(TogglerModel::Roles::Display)).toUInt();
        return display & m_displayFilter;
    }

    return true;
}
