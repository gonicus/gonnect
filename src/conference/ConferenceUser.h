#pragma once

#include <QObject>
#include <qqmlregistration.h>

class ConferenceUser : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName CONSTANT FINAL)
    Q_PROPERTY(ConferenceUser::Role role READ role NOTIFY roleChanged FINAL)

public:
    enum class Role { None, User, Moderator };
    Q_ENUM(Role)

    static QString userRoleToString(const Role role);

    explicit ConferenceUser(const QString &id, const QString &displayName, Role role,
                            QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    Role role() const { return m_role; }

    void setRole(Role newRole);

Q_SIGNALS:
    Q_EMIT void roleChanged();

private:
    QString m_id;
    QString m_displayName;
    Role m_role = Role::None;
};
