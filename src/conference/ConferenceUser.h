#pragma once

#include <QObject>
#include <QDateTime>
#include <qqmlregistration.h>

class ConferenceUser : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged FINAL)
    Q_PROPERTY(ConferenceUser::Role role READ role NOTIFY roleChanged FINAL)
    Q_PROPERTY(bool isAudioMuted READ isAudioMuted NOTIFY isAudioMutedChanged FINAL)
    Q_PROPERTY(bool isVideoMuted READ isVideoMuted NOTIFY isVideoMutedChanged FINAL)
    Q_PROPERTY(QDateTime handRaisedDateTime READ handRaisedDateTime NOTIFY handRaisedDateTimeChanged
                       FINAL)
    Q_PROPERTY(bool isHandRaised READ isHandRaised NOTIFY handRaisedDateTimeChanged FINAL)
    Q_PROPERTY(bool isDominantSpeaker READ isDominantSpeaker NOTIFY isDominantSpeakerChanged FINAL)

public:
    enum class Role { None, User, Moderator };
    Q_ENUM(Role)

    static QString userRoleToString(const Role role);

    explicit ConferenceUser(const QString &id, const QString &displayName, Role role,
                            QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    Role role() const { return m_role; }
    bool isAudioMuted() const { return m_isAudioMuted; }
    bool isVideoMuted() const { return m_isVideoMuted; }
    QDateTime handRaisedDateTime() const { return m_handRaisedDateTime; }
    bool isHandRaised() const { return m_handRaisedDateTime.isValid(); }
    bool isDominantSpeaker() const { return m_isDominantSpeaker; }

    void setRole(Role newRole);
    void setDisplayName(const QString &displayName);
    void setAudioMuted(bool isMuted);
    void setVideoMuted(bool isMuted);
    void setHandRaisedDateTime(const QDateTime &dateTime);
    void setDominantSpeaker(bool isDominantSpeaker);

Q_SIGNALS:
    Q_EMIT void roleChanged();
    void displayNameChanged();
    void isAudioMutedChanged();
    void isVideoMutedChanged();
    void handRaisedDateTimeChanged();
    void isDominantSpeakerChanged();

private:
    QString m_id;
    QString m_displayName;
    Role m_role = Role::None;

    bool m_isAudioMuted = true;
    bool m_isVideoMuted = true;

    QDateTime m_handRaisedDateTime;
    bool m_isDominantSpeaker = false;
};
