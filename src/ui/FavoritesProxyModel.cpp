#include "FavoritesProxyModel.h"
#include "FavoritesModel.h"
#include "NumberStats.h"
#include "IChatRoom.h"

FavoritesProxyModel::FavoritesProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &FavoritesProxyModel::showPhoneChanged, this,
            &FavoritesProxyModel::triggerUpdate);
    connect(this, &FavoritesProxyModel::showJitsiChanged, this,
            &FavoritesProxyModel::triggerUpdate);
    connect(this, &FavoritesProxyModel::showChatRoomsChanged, this,
            &FavoritesProxyModel::triggerUpdate);
}

void FavoritesProxyModel::triggerUpdate()
{
    beginFilterChange();
    endFilterChange();

    if (rowCount() > 0) {
        Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0),
                           { static_cast<int>(FavoritesModel::Roles::Addresses) });
    }
}

QVariant FavoritesProxyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if ((!m_showPhone || !m_showJitsi || !m_showChatRooms)
        && role == static_cast<int>(FavoritesModel::Roles::Addresses)) {
        const QVariantList originalAddrs = mapToSource(index).data(role).toList();
        QVariantList addrs;
        addrs.reserve(originalAddrs.length());

        for (const auto &addr : originalAddrs) {
            if ((m_showPhone || !isPhoneAddr(addr.toMap()))
                && (m_showJitsi || !isJitsiAddr(addr.toMap()))
                && (m_showChatRooms || !isChatAddr(addr.toMap()))) {
                addrs.append(addr);
            }
        }

        return addrs;
    }

    return QSortFilterProxyModel::data(index, role);
}

bool FavoritesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    using Roles = FavoritesModel::Roles;

    const auto model = qobject_cast<FavoritesModel *>(sourceModel());
    if (!model) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);

    if (!m_showPhone || !m_showJitsi || !m_showChatRooms) {
        const auto addrs = model->data(index, static_cast<int>(Roles::Addresses)).toList();

        for (const auto &addr : addrs) {
            const auto map = addr.toMap();
            if ((m_showPhone || !isPhoneAddr(map)) && (m_showJitsi || !isJitsiAddr(map))
                && (m_showChatRooms || !isChatAddr(map))) {
                return true;
            }
        }
        return false;
    }

    return true;
}

bool FavoritesProxyModel::isPhoneAddr(const QVariantMap &addr) const
{
    using ContactType = NumberStats::ContactType;
    return static_cast<ContactType>(addr.value("contactType", -1).toInt())
            == ContactType::PhoneNumber;
}

bool FavoritesProxyModel::isJitsiAddr(const QVariantMap &addr) const
{
    using ContactType = NumberStats::ContactType;
    return static_cast<ContactType>(addr.value("contactType", -1).toInt())
            == ContactType::JitsiMeetUrl;
}

bool FavoritesProxyModel::isChatAddr(const QVariantMap &addr) const
{
    return addr.value("chatRoom").value<IChatRoom *>();
}
