#include "EmojiProxyModel.h"
#include "EmojiModel.h"

EmojiProxyModel::EmojiProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &EmojiProxyModel::groupChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });

    connect(this, &EmojiProxyModel::filterTextChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });
}

bool EmojiProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = qobject_cast<EmojiModel *>(sourceModel());
    if (!model) {
        return false;
    }

    using Roles = EmojiModel::Roles;
    const auto index = model->index(sourceRow, 0, sourceParent);

    // Group filter
    if (m_group >= 0) {

        const auto group = model->data(index, static_cast<int>(Roles::Group)).toInt();
        if (group != m_group) {
            return false;
        }
    }

    // Text filter
    const auto trimmed = m_filterText.trimmed();
    if (!trimmed.isEmpty()) {
        const auto label = model->data(index, static_cast<int>(Roles::Label)).toString();
        if (label.contains(trimmed, Qt::CaseInsensitive)) {
            return true;
        }

        const auto tags = model->data(index, static_cast<int>(Roles::Tags)).toStringList();
        for (const auto &tag : tags) {
            if (tag.contains(trimmed, Qt::CaseInsensitive)) {
                return true;
            }
        }

        return false;
    }

    return true;
}

int EmojiProxyModel::firstIndexOfGroup(int groupIndex) const
{
    const auto model = qobject_cast<const EmojiModel *>(sourceModel());
    if (!model) {
        return -1;
    }

    const int count = rowCount();
    for (int i = 0; i < count; ++i) {
        const auto sourceIdx = mapToSource(index(i, 0));
        if (!sourceIdx.isValid()) {
            continue;
        }
        if (model->groupAt(sourceIdx.row()) == groupIndex) {
            return i;
        }
    }
    return -1;
}
