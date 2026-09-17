#pragma once

#include <QObject>
#include <QQmlEngine>

class ChatMessageContentUserStateChange : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(ChatMessageContentUserStateChange::State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString affectedUserId READ affectedUserId NOTIFY affectedUserIdChanged FINAL)

public:
    enum class State { Unknown = 0, Joined, Left, Invited, Knocked, Banned, Unbanned, Kicked };
    Q_ENUM(State)

    explicit ChatMessageContentUserStateChange(State state = State::Unknown,
                                               const QString &affectedUserId = "",
                                               QObject *parent = nullptr);

    State state() const { return m_state; }
    void setSetState(const State state);

    QString affectedUserId() const { return m_affectedUserId; }
    void setAffectedUserId(const QString &id);

private:
    State m_state = State::Unknown;
    QString m_affectedUserId;

Q_SIGNALS:
    void stateChanged();
    void affectedUserIdChanged();
};
