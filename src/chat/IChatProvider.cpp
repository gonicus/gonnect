#include "IChatProvider.h"

IChatProvider::IChatProvider(const QString &settingsGroup, QObject *parent)
    : QObject{ parent }, m_settingsGroup{ settingsGroup }
{
}

void IChatProvider::setIsConnected(bool value)
{
    if (m_isConnected != value) {
        m_isConnected = value;
        emit isConnectedChanged();
    }
}
