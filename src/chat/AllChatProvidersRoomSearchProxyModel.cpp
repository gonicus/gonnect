#include "AllChatProvidersRoomSearchProxyModel.h"
#include "ChatRoomModel.h"

AllChatProvidersRoomSearchProxyModel::AllChatProvidersRoomSearchProxyModel(QObject *parent)
    : QSortFilterProxyModel{ parent }
{
    connect(this, &AllChatProvidersRoomSearchProxyModel::filterTextChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });
}

bool AllChatProvidersRoomSearchProxyModel::filterAcceptsRow(int sourceRow,
                                                            const QModelIndex &sourceParent) const
{
    const auto *model = sourceModel();
    const auto filterStr = m_filterText.trimmed();
    if (!model || filterStr.isEmpty()) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);
    using Roles = ChatRoomModel::Roles;

    const auto roomName = model->data(index, static_cast<int>(Roles::Name)).toString();
    const auto id = model->data(index, static_cast<int>(Roles::RoomId));

    return roomName.contains(filterStr, Qt::CaseSensitivity::CaseInsensitive);
}
