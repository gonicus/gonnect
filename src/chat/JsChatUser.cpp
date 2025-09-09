#include "JsChatUser.h"

JsChatUser::JsChatUser(const QString &userId, const QString &displayName, QObject *parent)
    : QObject{ parent }, m_userId{ userId }, m_displayName{ displayName }
{
}
