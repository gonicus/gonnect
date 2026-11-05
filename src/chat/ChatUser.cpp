#include "ChatUser.h"
#include "IChatProvider.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcChatUser, "gonnect.app.chat.user")

ChatUser::ChatUser(const QString &id, const QString &displayName, bool hasPresenceState,
                   QString avatarPath, IChatProvider *parent)
    : QObject{ parent },
      m_id{ id },
      m_displayName{ displayName },
      m_avatarPath{ avatarPath },
      m_hasPresenceState{ hasPresenceState },
      m_chatProvider{ parent }
{
}

void ChatUser::setDisplayName(const QString &name)
{
    if (m_displayName != name) {
        m_displayName = name;
        Q_EMIT displayNameChanged(name);
    }
}

QString ChatUser::computedName() const
{
    return m_displayName.isEmpty() ? m_id : m_displayName;
}

void ChatUser::setAvatarPath(const QString &newPath)
{
    if (m_avatarPath != newPath) {
        m_avatarPath = newPath;
        Q_EMIT avatarPathChanged();
    }
}

void ChatUser::setHasPresenceState(const bool value)
{
    if (m_hasPresenceState != value) {
        m_hasPresenceState = value;
        Q_EMIT hasPresenceStateChanged();
    }
}

void ChatUser::setPresenceState(const PresenceState state)
{
    if (m_presenceState != state) {
        m_presenceState = state;
        Q_EMIT presenceStateChanged();
    }
}

QDebug operator<<(QDebug debug, const ChatUser &user)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "ChatUser("
                              << "id=" << user.id() << ","
                              << "displayName=" << user.displayName() << ")";
    return debug;
}
