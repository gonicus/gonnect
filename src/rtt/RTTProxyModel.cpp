#include "RTTProxyModel.h"
#include "RTTModel.h"

RTTProxyModel::RTTProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

bool RTTProxyModel::showOnlyFinished() const
{
    return m_showOnlyFinished;
}

void RTTProxyModel::setShowOnlyFinished(bool showFinished)
{
    if (m_showOnlyFinished != showFinished) {
        beginFilterChange();
        m_showOnlyFinished = showFinished;
        endFilterChange();

        Q_EMIT showOnlyFinishedChanged();
    }
}

bool RTTProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!sourceModel()) {
        return true;
    }

    if (!m_showOnlyFinished) {
        return true;
    }

    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

    return sourceModel()->data(idx, static_cast<int>(RTTModel::Roles::IsFinished)).toBool();
}
