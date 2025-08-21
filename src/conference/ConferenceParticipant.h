#pragma once

#include <QObject>
#include <qqmlregistration.h>

class ConferenceParticipant : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class Role { None, Participant, Moderator };
    Q_ENUM(Role)

    explicit ConferenceParticipant(const QString &id, const QString &displayName, Role role,
                                   QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    Role role() const { return m_role; }

    void setRole(Role newRole);

signals:
    emit void roleChanged();

private:
    QString m_id;
    QString m_displayName;
    Role m_role = Role::None;
};
