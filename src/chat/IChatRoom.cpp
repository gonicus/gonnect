#include "IChatRoom.h"

IChatRoom::IChatRoom(QObject *parent) : QObject{ parent } { }

void IChatRoom::setIsLoadingMessageHistory(bool value)
{
    if (m_isLoadingMessageHistory != value) {
        m_isLoadingMessageHistory = value;
        Q_EMIT isLoadingMessageHistoryChanged();
    }
}

void IChatRoom::setIsCompletelyLoaded(bool value)
{
    if (m_isCompletelyLoaded != value) {
        m_isCompletelyLoaded = value;
        Q_EMIT isCompletelyLoadedChanged();
    }
}

void IChatRoom::setLatestMessageDateTime(const QDateTime &dateTime)
{
    if (m_latestMessageDateTime != dateTime) {
        m_latestMessageDateTime = dateTime;
        Q_EMIT latestMessageDateTimeChanged();
    }
}
