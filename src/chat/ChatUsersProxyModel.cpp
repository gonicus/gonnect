#include "ChatUsersProxyModel.h"
#include "ChatUsersModel.h"

ChatUsersProxyModel::ChatUsersProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &ChatUsersProxyModel::selectedUserIdsChanged, this, [this]() { invalidate(); });
    connect(this, &ChatUsersProxyModel::excludedUserIdsChanged, this, [this]() { invalidate(); });
    connect(this, &ChatUsersProxyModel::filterTextChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });

    sort(0);
}

void ChatUsersProxyModel::toggleSelectedState(const QString &id)
{
    const auto idx = m_selectedUserIds.indexOf(id);
    if (idx < 0) {
        m_selectedUserIds.append(id);
    } else {
        m_selectedUserIds.removeAt(idx);
    }

    Q_EMIT selectedUserIdsChanged();
}

bool ChatUsersProxyModel::lessThan(const QModelIndex &sourceLeft,
                                   const QModelIndex &sourceRight) const
{
    const auto model = qobject_cast<ChatUsersModel *>(sourceModel());
    if (!model) {
        return false;
    }

    using Roles = ChatUsersModel::Roles;

    const auto &leftId = model->data(sourceLeft, static_cast<int>(Roles::Id)).toString();
    const auto &rightId = model->data(sourceRight, static_cast<int>(Roles::Id)).toString();
    const auto &leftName = model->data(sourceLeft, static_cast<int>(Roles::Name)).toString();
    const auto &rightName = model->data(sourceRight, static_cast<int>(Roles::Name)).toString();
    const bool isLeftSelected = m_selectedUserIds.contains(leftId);
    const bool isRightSelected = m_selectedUserIds.contains(rightId);

    if (isLeftSelected && !isRightSelected) {
        return true;
    }
    if (!isLeftSelected && isRightSelected) {
        return false;
    }

    return leftName.localeAwareCompare(rightName) < 0;
}

bool ChatUsersProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    const auto model = qobject_cast<ChatUsersModel *>(sourceModel());
    if (!model) {
        return false;
    }

    if (m_filterText.isEmpty() && m_excludedUserIds.isEmpty()) {
        return true;
    }

    using Roles = ChatUsersModel::Roles;
    const auto idx = createIndex(sourceRow, 0);

    const auto &id = model->data(idx, static_cast<int>(Roles::Id)).toString();
    if (m_excludedUserIds.contains(id)) {
        return false;
    }
    if (m_selectedUserIds.contains(id)) {
        return true;
    }

    const auto &name = model->data(idx, static_cast<int>(Roles::Name)).toString();
    return name.contains(m_filterText, Qt::CaseSensitivity::CaseInsensitive);
}
