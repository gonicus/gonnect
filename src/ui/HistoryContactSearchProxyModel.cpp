#include "HistoryContactSearchProxyModel.h"
#include "HistoryContactSearchModel.h"

HistoryContactSearchProxyModel::HistoryContactSearchProxyModel(QObject *parent)
    : QSortFilterProxyModel{ parent }
{
    connect(this, &HistoryContactSearchProxyModel::showJitsiChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });
}

bool HistoryContactSearchProxyModel::filterAcceptsRow(int sourceRow,
                                                      const QModelIndex &sourceParent) const
{
    const auto model = qobject_cast<HistoryContactSearchModel *>(sourceModel());
    if (!model) {
        return false;
    }

    using Roles = HistoryContactSearchModel::Roles;

    const auto index = model->index(sourceRow, 0, sourceParent);

    if (!m_showJitsi && !model->data(index, static_cast<int>(Roles::IsPhoneNumber)).toBool()) {
        return false;
    }

    return true;
}
