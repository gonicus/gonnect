#pragma once

#include <QString>
#include <QList>
#include <QPointer>

#include "Contact.h"

struct ContactInfo
{
    QString sipUrl;
    QString phoneNumber;
    QString displayName;
    Contact::NumberType numberType = Contact::NumberType::Unknown;
    QString city;
    QStringList countries;
    bool isAnonymous = false;
    bool isSipSubscriptable = false;
    QPointer<Contact> contact = nullptr;

    QString toString() const;

    bool operator==(const ContactInfo &other);
    bool operator!=(const ContactInfo &other);
};

QDebug operator<<(QDebug debug, const ContactInfo &contactInfo);

class PhoneNumberUtil : public QObject
{
    Q_OBJECT

public:
    static PhoneNumberUtil &instance()
    {
        static PhoneNumberUtil *_instance = nullptr;
        if (!_instance) {
            _instance = new PhoneNumberUtil;
        }
        return *_instance;
    }

    ContactInfo contactInfoBySipUrl(const QString &sipUrl);

    static QString cleanPhoneNumber(const QString &number);
    static QString clearInternationalChars(const QString &str);
    static bool isSipUri(const QString &str);
    static QString numberFromSipUrl(const QString &sipUrl);
    static bool isEmergencyCallUrl(const QString &sipUrl);
    static bool isNumberAnonymous(const QString &sipUrl);

private:
    PhoneNumberUtil();

    QHash<QString, ContactInfo> m_contactInfoCache;
};
