#include "EmojiProxyModel.h"
#include "EmojiModel.h"

EmojiProxyModel::EmojiProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &EmojiProxyModel::groupChanged, this, [this]() { invalidateFilter(); });
}

bool EmojiProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{

    const auto model = qobject_cast<EmojiModel *>(sourceModel());

    if (!model) {
        return false;
    }
    if (m_group < 0) {
        return true;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);
    const auto group = model->data(index, static_cast<int>(EmojiModel::Roles::Group)).toInt();

    return group == m_group;
}
