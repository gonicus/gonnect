#include "Contact.h"
#include "FuzzyCompare.h"
#include "PhoneNumberUtil.h"

#ifndef APP_TESTS
#  include "AvatarManager.h"
#endif
#include <QMetaEnum>

Contact::Contact(QObject *parent) : QObject{ parent } { }

Contact::Contact(const QString &id, const QString &dn, const QString &name, QObject *parent)
    : QObject{ parent }, m_id{ id }, m_dn{ dn }, m_name{ name }
{
    init();
}

Contact::Contact(const QString &id, const QString &dn, const QString &name, const QString &company,
                 const QString &mail, const QDateTime &lastModified,
                 const QList<Contact::PhoneNumber> &phoneNumbers, QObject *parent)
    : QObject{ parent },
      m_id{ id },
      m_dn{ dn },
      m_name{ name },
      m_company{ company },
      m_mail{ mail },
      m_lastModified{ lastModified },
      m_phoneNumbers{ phoneNumbers }
{

    init();
}

Contact::Contact(const Contact &other) : QObject{ other.parent() }
{
    m_id = other.m_id;
    m_dn = other.m_dn;
    m_name = other.m_name;
    m_company = other.m_company;
    m_phoneNumbers = other.m_phoneNumbers;
    m_mail = other.m_mail;
    m_lastModified = other.m_lastModified;
    m_sipStatusSubscriptable = other.m_sipStatusSubscriptable;
    init();
}

Contact &Contact::operator=(const Contact &other)
{
    m_id = other.m_id;
    m_dn = other.m_dn;
    m_name = other.m_name;
    m_company = other.m_company;
    m_phoneNumbers = other.m_phoneNumbers;
    m_mail = other.m_mail;
    m_lastModified = other.m_lastModified;
    m_sipStatusSubscriptable = other.m_sipStatusSubscriptable;
    init();
    return *this;
}

QString Contact::id() const
{
    return m_id;
}

QString Contact::dn() const
{
    return m_dn;
}

QString Contact::name() const
{
    return m_name;
}

QString Contact::company() const
{
    return m_company;
}

QString Contact::mail() const
{
    return m_mail;
}

bool Contact::hasAvatar() const
{
    return m_hasAvatar;
}

QString Contact::avatarPath() const
{
#ifndef APP_TESTS
    if (m_hasAvatar) {
        return AvatarManager::instance().avatarPathFor(m_id);
    }
#endif
    return "";
}

QDateTime Contact::lastModified() const
{
    return m_lastModified;
}

void Contact::addPhoneNumber(const Contact::NumberType type, const QString &phoneNumber,
                             bool isSipStatusSubscriptable)
{
    const auto clean = PhoneNumberUtil::cleanPhoneNumber(phoneNumber);

    if (isNumberValid(clean)
        && !m_phoneNumbers.contains({ type, clean, isSipStatusSubscriptable })) {
        m_phoneNumbers.append({ type, clean, isSipStatusSubscriptable });
    }

    updateSipStatusSubscriptable();
}

void Contact::addPhoneNumbers(const QList<PhoneNumber> &phoneNumbers)
{
    for (const auto &item : std::as_const(phoneNumbers)) {
        addPhoneNumber(item.type, item.number, item.isSipSubscriptable);
    }

    updateSipStatusSubscriptable();
}

Contact::PhoneNumber Contact::phoneNumberObject(const QString &phoneNumber) const
{
    for (const auto &item : std::as_const(m_phoneNumbers)) {
        if (item.number == phoneNumber) {
            return item;
        }
    }
    return PhoneNumber();
}

qreal Contact::matchesSearch(const QString &searchString) const
{
    qreal maxDist = 0;

    // Full name
    const auto fullName = m_splittedName.join(' ');
    if (searchString.compare(fullName, Qt::CaseSensitivity::CaseInsensitive) == 0) {
        return 1;
    }

    if (fullName.startsWith(searchString, Qt::CaseSensitivity::CaseInsensitive)) {
        return 0.98;
    }

    const qreal dist = FuzzyCompare::jaroWinklerDistance(fullName, searchString);
    maxDist = std::max(dist, maxDist);
    if (maxDist == 1.0) {
        return maxDist;
    }

    // Parts of the name
    for (const auto &str : std::as_const(m_splittedName)) {
        if (searchString.compare(str, Qt::CaseSensitivity::CaseInsensitive) == 0) {
            return 0.99;
        }

        if (str.startsWith(searchString, Qt::CaseSensitivity::CaseInsensitive)) {
            return 0.97;
        }

        const qreal dist = FuzzyCompare::jaroWinklerDistance(str, searchString);
        maxDist = std::max(dist, maxDist);
        if (maxDist == 1.0) {
            return maxDist;
        }
    }

    // Phone number
    for (const auto &phoneNumber : std::as_const(m_phoneNumbers)) {
        const qreal dist = FuzzyCompare::jaroWinklerDistance(phoneNumber.number, searchString);
        maxDist = std::max(dist, maxDist);
        if (maxDist == 1.0) {
            return maxDist;
        }
    }

    return maxDist;
}

void Contact::init()
{
    m_splittedName = PhoneNumberUtil::clearInternationalChars(m_name).toLower().split(' ');

    updateSipStatusSubscriptable();
}

void Contact::updateSipStatusSubscriptable()
{
    for (const auto &phoneNumber : std::as_const(m_phoneNumbers)) {
        if (phoneNumber.isSipSubscriptable) {
            m_sipStatusSubscriptable = true;
            return;
        }
    }
    m_sipStatusSubscriptable = false;
}

bool Contact::isNumberValid(const QString &number) const
{
    return !number.isEmpty() && number != "+492932916";
}

QList<Contact::PhoneNumber> Contact::phoneNumbers() const
{
    return m_phoneNumbers;
}

bool Contact::sipStatusSubscriptable() const
{
    return m_sipStatusSubscriptable;
}

QString Contact::subscriptableNumber() const
{
    if (!m_sipStatusSubscriptable) {
        return "";
    }

    for (const auto &phoneNumber : std::as_const(m_phoneNumbers)) {
        if (phoneNumber.isSipSubscriptable) {
            return phoneNumber.number;
        }
    }
    return "";
}

void Contact::setCompany(const QString &company)
{
    m_company = company;
}

void Contact::setMail(const QString &mail)
{
    m_mail = mail;
}

void Contact::setLastModified(const QDateTime &lastModified)
{
    m_lastModified = lastModified;
}

void Contact::setHasAvatar(bool hasAvatar)
{
    if (m_hasAvatar != hasAvatar) {
        m_hasAvatar = hasAvatar;
        emit avatarChanged();
    }
}

QDataStream &operator<<(QDataStream &out, const Contact &contact)
{
    out << contact.id() << contact.dn() << contact.name() << contact.company() << contact.mail()
        << contact.lastModified() << contact.sipStatusSubscriptable() << contact.phoneNumbers();
    return out;
}

QDataStream &operator>>(QDataStream &in, Contact &contact)
{
    QString id;
    QString dn;
    QString name;
    QString company;
    QString mail;
    QDateTime lastModified;
    bool sipStatusSubscriptable;
    QList<Contact::PhoneNumber> phoneNumbers;

    in >> id >> dn >> name >> company >> mail >> lastModified >> sipStatusSubscriptable
            >> phoneNumbers;
    contact = Contact(id, dn, name, company, mail, lastModified, phoneNumbers);

    return in;
}

QDebug operator<<(QDebug debug, const Contact &contact)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << contact.id() << contact.name();

    const auto phoneNumbers = contact.phoneNumbers();

    if (phoneNumbers.size()) {
        debug.nospace() << " (" << phoneNumbers << ")";
    }
    return debug;
}

QDataStream &operator<<(QDataStream &out, const Contact::PhoneNumber &phoneNumber)
{
    out << phoneNumber.type << phoneNumber.isSipSubscriptable << phoneNumber.number;
    return out;
}

QDataStream &operator>>(QDataStream &in, Contact::PhoneNumber &phoneNumber)
{
    Contact::NumberType type;
    QString number;
    bool isSipSubscriptable;

    in >> type >> isSipSubscriptable >> number;
    phoneNumber = { type, number, isSipSubscriptable };
    return in;
}

QDebug operator<<(QDebug debug, const Contact::PhoneNumber &phoneNumber)
{
    QDebugStateSaver saver(debug);
    QMetaEnum metaEnum = QMetaEnum::fromType<Contact::NumberType>();

    debug.nospace() << metaEnum.valueToKey(static_cast<int>(phoneNumber.type)) << "("
                    << phoneNumber.number << ")";

    return debug;
}

bool Contact::PhoneNumber::operator==(const PhoneNumber &other) const
{
    return type == other.type && number == other.number
            && isSipSubscriptable == other.isSipSubscriptable;
}

bool Contact::PhoneNumber::operator!=(const PhoneNumber &other) const
{
    return type != other.type || number != other.number
            || isSipSubscriptable != other.isSipSubscriptable;
}
