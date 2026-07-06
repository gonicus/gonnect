#pragma once

#include <QObject>
#include <QDebug>
#include <QDateTime>
#include <qqmlregistration.h>
#include "BlockInfo.h"
#include "PresenceStateAggregator.h"

class ChatUser;

class Contact : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(bool hasAvatar READ hasAvatar NOTIFY avatarChanged FINAL)
    Q_PROPERTY(QString avatarPath READ avatarPath NOTIFY avatarChanged FINAL)
    Q_PROPERTY(bool hasBuddyState READ sipStatusSubscriptable CONSTANT FINAL)
    Q_PROPERTY(QString subscriptableNumber READ subscriptableNumber CONSTANT FINAL)

public:
    enum class NumberType { Unknown, Commercial, Home, Mobile };
    Q_ENUM(NumberType)

    struct PhoneNumber
    {
        Contact::NumberType type;
        QString number;
        bool isSipSubscriptable = false;

        bool operator==(const PhoneNumber &other) const;
        bool operator!=(const PhoneNumber &other) const;
    };

    struct ContactSourceInfo
    {
        unsigned prio = 0;
        QString displayName;
        QString configId;

        bool operator==(const ContactSourceInfo &other) const;
        bool operator!=(const ContactSourceInfo &other) const;
    };

    explicit Contact(const QString &id, const QString &dn, const QString &sourceUid,
                     const ContactSourceInfo &contactSourceInfo, const QString &name,
                     BlockInfo blockInfo, QObject *parent = nullptr);
    explicit Contact(const QString &id, const QString &dn, const QString &sourceUid,
                     const ContactSourceInfo &contactSourceInfo, const QString &name,
                     const QString &company, const QString &mail, const QDateTime &lastModified,
                     const QList<Contact::PhoneNumber> &phoneNumbers, BlockInfo blockInfo,
                     QObject *parent = nullptr);

    explicit Contact(QObject *parent = nullptr);
    explicit Contact(const Contact &other);

    Contact &operator=(const Contact &other);

    QString id() const;
    QString dn() const;
    QString sourceUid() const;
    QString name() const;
    BlockInfo blockInfo() const;
    const ContactSourceInfo &contactSourceInfo() const;
    QString company() const;
    QString mail() const;
    bool hasAvatar() const;
    QString avatarPath() const;
    QDateTime lastModified() const;
    QList<Contact::PhoneNumber> phoneNumbers() const;
    bool sipStatusSubscriptable() const;
    QString subscriptableNumber() const;

    void setDisplayName(const QString &dn);
    void setName(const QString &name);
    void setCompany(const QString &company);
    void setMail(const QString &mail);
    void setLastModified(const QDateTime &lastModified);
    void setContactSourceInfo(const ContactSourceInfo &contactSourceInfo);
    void setHasAvatar(bool hasAvatar);
    void addPhoneNumber(const Contact::NumberType type, const QString &phoneNumber,
                        bool isSipStatusSubscriptable);
    void addPhoneNumbers(const QList<Contact::PhoneNumber> &phoneNumbers);

    void clearPhoneNumbers();

    Contact::PhoneNumber phoneNumberObject(const QString &phoneNumber) const;

    qreal matchesSearch(const QString &searchString) const;

#ifndef APP_TESTS
    bool hasChatUser(const ChatUser *user) const;
    void addChatUser(ChatUser *user);
    void removeChatUser(ChatUser *user);
    const QList<ChatUser *> &chatUsers() const { return m_chatUsers; }

    [[nodiscard("Caller must take ownership")]] PresenceStateAggregator *
    createPresenceStateObject() const;
#endif

private:
    void init();
    void updateSipStatusSubscriptable();
    bool isNumberValid(const QString &number) const;

    bool m_hasAvatar = false;
    BlockInfo m_blockInfo;
    QString m_id;
    QString m_dn;
    QString m_sourceUid;
    QString m_name;
    ContactSourceInfo m_contactSourceInfo;
    QString m_company;
    QString m_mail;
    QDateTime m_lastModified;
    QStringList m_splittedName;
    QString m_fullNameLower;
    QList<PhoneNumber> m_phoneNumbers;
    bool m_sipStatusSubscriptionInitialized = false;
    bool m_sipStatusSubscriptable = false;

    /// References to users of a chat plugin.
    QList<ChatUser *> m_chatUsers;

Q_SIGNALS:
    void avatarChanged();
};

QDataStream &operator<<(QDataStream &out, const Contact &contact);
QDataStream &operator>>(QDataStream &in, Contact &contact);
QDebug operator<<(QDebug debug, const Contact &contact);

QDataStream &operator<<(QDataStream &out, const Contact::PhoneNumber &phoneNumber);
QDataStream &operator>>(QDataStream &in, Contact::PhoneNumber &phoneNumber);
QDebug operator<<(QDebug debug, const Contact::PhoneNumber &phoneNumber);
