#include "ChatRoomProxyModel.h"
#include "ChatRoomModel.h"

ChatRoomProxyModel::ChatRoomProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &QSortFilterProxyModel::sourceModelChanged, this,
            &ChatRoomProxyModel::onSourceModelChanged);

    connect(this, &ChatRoomProxyModel::sortStrategyChanged, this, &ChatRoomProxyModel::applySort);

    applySort();
    sort(0);
}

QHash<int, QByteArray> ChatRoomProxyModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    if (const auto model = sourceModel()) {
        roles = model->roleNames();
    }
    roles[static_cast<int>(Roles::SectionHeader)] = "sectionHeader";
    return roles;
}

QVariant ChatRoomProxyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case static_cast<int>(Roles::SectionHeader): {

        if (!m_groupFavorites) {
            return "";
        }

        // Previous element is favorite, current is not
        using Roles = ChatRoomModel::Roles;

        if (index.row() == 0) {
            if (mapToSource(index).data(static_cast<int>(Roles::IsFavorite)).toBool()) {
                return tr("Favorites");
            } else {
                return tr("Others");
            }
        }

        if (!mapToSource(index).data(static_cast<int>(Roles::IsFavorite)).toBool()) {
            QModelIndex prevIndex = index.sibling(index.row() - 1, 0);

            if (prevIndex.isValid()
                && mapToSource(prevIndex).data(static_cast<int>(Roles::IsFavorite)).toBool()) {
                return tr("Others");
            }
        }
        return "";
    }

    default:
        return QSortFilterProxyModel::data(index, role);
    }
}

void ChatRoomProxyModel::setOnlyUnread(bool value)
{
    if (m_onlyUnread != value) {
        beginFilterChange();
        m_onlyUnread = value;
        endFilterChange(Direction::Rows);
        Q_EMIT onlyUnreadChanged();
    }
}

void ChatRoomProxyModel::setGroupFavorites(bool value)
{
    if (m_groupFavorites != value) {
        Q_EMIT layoutAboutToBeChanged();
        m_groupFavorites = value;
        this->invalidate();
        Q_EMIT layoutChanged();
        Q_EMIT groupFavoritesChanged();
    }
}

void ChatRoomProxyModel::setFilterText(const QString &filterText)
{
    if (m_filterText != filterText) {
        beginFilterChange();
        m_filterText = filterText;
        endFilterChange(Direction::Rows);
        Q_EMIT filterTextChanged();
    }
}

bool ChatRoomProxyModel::lessThan(const QModelIndex &sourceLeft,
                                  const QModelIndex &sourceRight) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    using Roles = ChatRoomModel::Roles;

    if (m_groupFavorites) {
        const auto leftIsFavorite =
                model->data(sourceLeft, static_cast<int>(Roles::IsFavorite)).toBool();
        const auto rightIsFavorite =
                model->data(sourceRight, static_cast<int>(Roles::IsFavorite)).toBool();

        if (leftIsFavorite && !rightIsFavorite) {
            return true;
        }
        if (rightIsFavorite && !leftIsFavorite) {
            return false;
        }
    }

    if (m_sortStrategy == SortStrategy::LatestActivity) {
        const auto leftTime =
                model->data(sourceLeft, static_cast<int>(Roles::LatestMessageDate)).toDateTime();
        const auto rightTime =
                model->data(sourceRight, static_cast<int>(Roles::LatestMessageDate)).toDateTime();

        if (leftTime.isValid() && rightTime.isValid()) {
            return leftTime > rightTime;
        }
        if (leftTime.isValid()) {
            return true;
        }
        if (rightTime.isValid()) {
            return false;
        }
    }

    const auto leftName = model->data(sourceLeft, static_cast<int>(Roles::Name)).toString();
    const auto rightName = model->data(sourceRight, static_cast<int>(Roles::Name)).toString();

    return leftName.localeAwareCompare(rightName) < 0;
}

bool ChatRoomProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    using Roles = ChatRoomModel::Roles;
    const auto idx = createIndex(sourceRow, 0);

    const auto ownJoinState = qvariant_cast<IChatRoom::UserRoomState>(
            model->data(idx, static_cast<int>(Roles::OwnJoinState)));

    if (m_onlyUnread) {
        const auto unreadCount =
                model->data(idx, static_cast<int>(Roles::UnreadCount)).toLongLong();

        if (unreadCount <= 0 && ownJoinState == IChatRoom::UserRoomState::Joined) {
            return false;
        }
    }

    if (!m_filterText.isEmpty()) {
        const auto id = model->data(idx, static_cast<int>(Roles::RoomId)).toString();
        const auto name = model->data(idx, static_cast<int>(Roles::Name)).toString();

        if (!id.contains(m_filterText, Qt::CaseSensitivity::CaseInsensitive)
            && !name.contains(m_filterText, Qt::CaseSensitivity::CaseInsensitive)) {
            return false;
        }
    }

    return ownJoinState != IChatRoom::UserRoomState::Unjoined;
}

void ChatRoomProxyModel::onSourceModelChanged()
{
    if (m_dataChangedConnection) {
        QObject::disconnect(m_dataChangedConnection);
        m_dataChangedConnection = QMetaObject::Connection();
    }

    if (auto model = sourceModel()) {
        using Roles = ChatRoomModel::Roles;
        m_dataChangedConnection =
                connect(model, &QAbstractListModel::dataChanged, this,
                        [this](const QModelIndex &, const QModelIndex &, const QList<int> &roles) {
                            if (roles.isEmpty()) {
                                invalidate();
                                sort(0);
                                return;
                            }

                            const bool isSortRole =
                                    std::any_of(roles.cbegin(), roles.cend(), [](const int role) {
                                        static const QSet<int> sortRoles = {
                                            static_cast<int>(Roles::IsFavorite),
                                            static_cast<int>(Roles::LatestMessageDate),
                                            static_cast<int>(Roles::Name),
                                            static_cast<int>(Roles::UnreadCount),
                                            static_cast<int>(Roles::OwnJoinState),
                                            static_cast<int>(Roles::RoomId),
                                        };
                                        return sortRoles.contains(role);
                                    });

                            if (isSortRole) {
                                invalidate();
                                sort(0);
                            }
                        });
    }
    sort(0);
}

void ChatRoomProxyModel::applySort()
{
    using Roles = ChatRoomModel::Roles;

    switch (m_sortStrategy) {
    case SortStrategy::Alphabetical:
        setSortRole(static_cast<int>(Roles::Name));
        break;
    case SortStrategy::LatestActivity:
        setSortRole(static_cast<int>(Roles::LatestMessageDate));
        break;
    }
}
