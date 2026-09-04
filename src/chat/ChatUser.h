#pragma once

#include <QObject>
#include <qqmlintegration.h>

class IChatProvider;

class ChatUser : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString computedName READ computedName NOTIFY displayNameChanged)
    Q_PROPERTY(bool hasPresenceState READ hasPresenceState NOTIFY hasPresenceStateChanged)
    Q_PROPERTY(ChatUser::PresenceState presenceState READ presenceState NOTIFY presenceStateChanged)

public:
    enum class PresenceState { Unknown, Offline, Away, Online };
    Q_ENUM(PresenceState)

    explicit ChatUser(const QString &id, const QString &displayName, bool hasPresenceState,
                      QString avatarPath, IChatProvider *parent);

    QString id() const { return m_id; }

    /// A custom name for of the user.
    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &name);

    /// Convenience function which returns the displayName() if set or id() otherwise.
    QString computedName() const;

    QString avatarPath() const { return m_avatarPath; }
    void setAvatarPath(const QString &newPath);

    IChatProvider *chatProvider() const { return m_chatProvider; }

    bool hasPresenceState() const { return m_hasPresenceState; }
    void setHasPresenceState(const bool value);
    PresenceState presenceState() const { return m_presenceState; }
    void setPresenceState(const PresenceState state);

private:
    QString m_id;
    QString m_displayName;
    QString m_avatarPath;
    bool m_hasPresenceState = false;
    PresenceState m_presenceState = PresenceState::Unknown;
    IChatProvider *m_chatProvider = nullptr;

Q_SIGNALS:
    void displayNameChanged(QString name);
    void hasPresenceStateChanged();
    void presenceStateChanged();
    void avatarPathChanged();
};

QDebug operator<<(QDebug debug, const ChatUser &user);
