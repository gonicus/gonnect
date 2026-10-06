#include "ConferenceLinkedRoomsProxyModel.h"
#include "ChatRoomModel.h"
#include "VideoCallHelper.h"
#include "IChatProvider.h"

ConferenceLinkedRoomsProxyModel::ConferenceLinkedRoomsProxyModel(QObject *parent)
    : QSortFilterProxyModel{ parent }
{
    sort(0);
}

void ConferenceLinkedRoomsProxyModel::setConferenceUrl(const QString &conferenceUrl)
{
    if (m_conferenceUrl == conferenceUrl) {
        return;
    }

    beginFilterChange();
    m_conferenceUrl = conferenceUrl;
    endFilterChange();

    Q_EMIT conferenceUrlChanged();
}

QString ConferenceLinkedRoomsProxyModel::nameAt(int row) const
{
    return data(index(row, 0), static_cast<int>(ChatRoomModel::Roles::Name)).toString();
}

IChatRoom *ConferenceLinkedRoomsProxyModel::chatRoomAt(int row) const
{
    const auto roomId =
            data(index(row, 0), static_cast<int>(ChatRoomModel::Roles::RoomId)).toString();
    const auto *provider = chatProviderAt(row);
    return provider ? provider->chatRoomByRoomId(roomId) : nullptr;
}

IChatProvider *ConferenceLinkedRoomsProxyModel::chatProviderAt(int row) const
{
    return qvariant_cast<IChatProvider *>(
            data(index(row, 0), static_cast<int>(ChatRoomModel::Roles::ChatProvider)));
}

bool ConferenceLinkedRoomsProxyModel::filterAcceptsRow(int sourceRow,
                                                       const QModelIndex &sourceParent) const
{
    if (m_conferenceUrl.isEmpty()) {
        return false;
    }

    const auto *model = sourceModel();
    if (!model) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);
    using Roles = ChatRoomModel::Roles;

    const auto roomConferenceUrl =
            model->data(index, static_cast<int>(Roles::ConferenceUrl)).toString();
    return VideoCallHelper::instance().urlsEquivalent(roomConferenceUrl, m_conferenceUrl);
}
