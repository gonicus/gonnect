#pragma once

#include <QObject>
#include <QDebug>
#include <QDateTime>
#include <qqmlregistration.h>

class Contact : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    QML_ELEMENT
    QML_UNCREATABLE("")

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

    explicit Contact(const QString &id, const QString &dn, const QString &name,
                     QObject *parent = nullptr);
    explicit Contact(const QString &id, const QString &dn, const QString &name,
                     const QString &company, const QString &mail, const QDateTime &lastModified,
                     const QList<Contact::PhoneNumber> &phoneNumbers, QObject *parent = nullptr);
    explicit Contact(const Contact &other);

    Contact &operator=(const Contact &other);

    QString id() const;
    QString dn() const;
    QString name() const;
    QString company() const;
    QString mail() const;
    bool hasAvatar() const;
    QString avatarPath() const;
    QDateTime lastModified() const;
    QList<Contact::PhoneNumber> phoneNumbers() const;
    bool sipStatusSubscriptable() const;
    QString subscriptableNumber() const;

    void setCompany(const QString &company);
    void setMail(const QString &mail);
    void setLastModified(const QDateTime &lastModified);
    void setHasAvatar(bool hasAvatar);
    void addPhoneNumber(const Contact::NumberType type, const QString &phoneNumber,
                        bool isSipStatusSubscriptable);
    void addPhoneNumbers(const QList<Contact::PhoneNumber> &phoneNumbers);

    Contact::PhoneNumber phoneNumberObject(const QString &phoneNumber) const;

    qreal matchesSearch(const QString &searchString) const;

private:
    void init();
    void updateSipStatusSubscriptable();
    bool isNumberValid(const QString &number) const;

    bool m_hasAvatar = false;
    QString m_id;
    QString m_dn;
    QString m_name;
    QString m_company;
    QString m_mail;
    QDateTime m_lastModified;
    QStringList m_splittedName;
    QList<PhoneNumber> m_phoneNumbers;
    bool m_sipStatusSubscriptable;

signals:
    void avatarChanged();
};

QDataStream &operator<<(QDataStream &out, const Contact &contact);
QDataStream &operator>>(QDataStream &in, Contact &contact);
QDebug operator<<(QDebug debug, const Contact &contact);

QDataStream &operator<<(QDataStream &out, const Contact::PhoneNumber &phoneNumber);
QDataStream &operator>>(QDataStream &in, Contact::PhoneNumber &phoneNumber);
QDebug operator<<(QDebug debug, const Contact::PhoneNumber &phoneNumber);
