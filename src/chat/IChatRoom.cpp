#include "IChatRoom.h"

IChatRoom::IChatRoom(IChatProvider *chatProvider, QObject *parent)
    : QObject{ parent }, m_chatProvider{ chatProvider }
{
}

void IChatRoom::setIsLoadingMessageHistory(bool value)
{
    if (m_isLoadingMessageHistory != value) {
        m_isLoadingMessageHistory = value;
        Q_EMIT isLoadingMessageHistoryChanged();
    }
}

void IChatRoom::setLatestMessageDateTime(const QDateTime &dateTime)
{
    if (m_latestMessageDateTime != dateTime) {
        m_latestMessageDateTime = dateTime;
        Q_EMIT latestMessageDateTimeChanged();
    }
}

void IChatRoom::setRoomSettings(const RoomSettings &roomSettings)
{
    if (m_roomSettings != roomSettings) {
        m_roomSettings = roomSettings;
        Q_EMIT roomSettingsChanged();
    }
}
