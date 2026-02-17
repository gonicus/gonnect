#include "SearchListProxyModel.h"
#include "SearchListModel.h"

SearchListProxyModel::SearchListProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &SearchListProxyModel::sourceDisplayNameChanged, this, [this]() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        endFilterChange();
#else
        invalidateRowsFilter();
#endif
    });
}

bool SearchListProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &parentIndex) const
{
    auto model = sourceModel();
    if (!model) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, parentIndex);
    const auto displayName =
            model->data(index, static_cast<int>(SearchListModel::Roles::SourceDisplayName))
                    .toString();

    return displayName == m_sourceDisplayName;
}
