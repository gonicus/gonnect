#include "MatrixRoom.h"

MatrixRoom::MatrixRoom(const QString &roomId, const QString name, QObject *parent)
    : QObject{ parent }, m_roomId{ roomId }, m_name{ name }
{
}

void MatrixRoom::setUnreadCount(quint16 value)
{
    if (m_unreadCount != value) {
        m_unreadCount = value;
        emit unreadCountChanged();
    }
}
