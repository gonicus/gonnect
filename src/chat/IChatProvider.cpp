#include "IChatProvider.h"

IChatProvider::IChatProvider(const QString &settingsGroup, QObject *parent)
    : QObject{ parent }, m_settingsGroup{ settingsGroup }
{
}

void IChatProvider::setIsConnected(bool value)
{
    if (m_isConnected != value) {
        m_isConnected = value;
        Q_EMIT isConnectedChanged();
    }
}

void IChatProvider::setUnreadNotificationsCount(qsizetype count)
{
    if (m_unreadNotificationsCount != count) {
        m_unreadNotificationsCount = count;
        Q_EMIT unreadNotificationsCountChanged();
    }
}

void IChatProvider::setIsSendingMessage(bool value)
{
    if (m_isSendingMessage != value) {
        m_isSendingMessage = value;
        Q_EMIT isSendingMessageChanged();
    }
}
