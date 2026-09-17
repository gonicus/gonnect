#include "FavoritesProxyModel.h"
#include "FavoritesModel.h"
#include "NumberStats.h"
#include "IChatRoom.h"

FavoritesProxyModel::FavoritesProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &FavoritesProxyModel::showJitsiChanged, this, [this]() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        endFilterChange();
#else
        invalidateRowsFilter();
#endif
    });
    connect(this, &FavoritesProxyModel::showChatRoomsChanged, this, [this]() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        endFilterChange();
#else
        invalidateRowsFilter();
#endif
    });
}

QVariant FavoritesProxyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if ((!m_showJitsi || !m_showChatRooms)
        && role == static_cast<int>(FavoritesModel::Roles::Addresses)) {
        const QVariantList originalAddrs = mapToSource(index).data(role).toList();
        QVariantList addrs;
        addrs.reserve(originalAddrs.length());

        for (const auto &addr : originalAddrs) {
            if ((m_showJitsi || !isJitsiAddr(addr.toMap()))
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

    if (!m_showJitsi || !m_showChatRooms) {
        const auto addrs = model->data(index, static_cast<int>(Roles::Addresses)).toList();
        bool hasNonJitsiAddr = false;
        bool hasNonChatAddr = false;
        for (const auto &addr : addrs) {
            if (!isJitsiAddr(addr.toMap())) {
                hasNonJitsiAddr = true;
            }
            if (!isChatAddr(addr.toMap())) {
                hasNonChatAddr = true;
            }
            if (hasNonChatAddr && hasNonJitsiAddr) {
                break;
            }
        }

        if ((!m_showJitsi && !hasNonJitsiAddr) || (!m_showChatRooms && !hasNonChatAddr)) {
            return false;
        }
    }

    return true;
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
