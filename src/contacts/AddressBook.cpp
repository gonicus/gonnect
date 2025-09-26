#include "AddressBook.h"
#include "Contact.h"
#include "FuzzyCompare.h"
#include "PhoneNumberUtil.h"

#include <QCryptographicHash>
#include <QRegularExpression>

AddressBook::AddressBook(QObject *parent) : QObject{ parent }
{
    connect(this, &AddressBook::contactAdded, this, &AddressBook::updateSourceInfos);
    connect(this, &AddressBook::contactsCleared, this, [this]() {
        if (m_contactSourceInfos.size()) {
            m_contactSourceInfos.clear();
            Q_EMIT contactSourceInfosChanged();
        }
    });
}

void AddressBook::updateSourceInfos(const Contact *contact)
{
    if (!contact) {
        return;
    }

    bool exists = false;
    for (const auto &info : std::as_const(m_contactSourceInfos)) {
        if (info == contact->contactSourceInfo()) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        m_contactSourceInfos.append(contact->contactSourceInfo());
        std::sort(m_contactSourceInfos.begin(), m_contactSourceInfos.end(),
                  [](const Contact::ContactSourceInfo &left,
                     const Contact::ContactSourceInfo &right) {
                      if (left.prio == right.prio) {
                          return left.displayName.localeAwareCompare(right.displayName) < 0;
                      }
                      return left.prio > right.prio;
                  });
        Q_EMIT contactSourceInfosChanged();
    }
}

QString AddressBook::hashifyCn(const QString &cn) const
{
    return QCryptographicHash::hash(cn.toUtf8(), QCryptographicHash::Sha256).toHex();
}

Contact *AddressBook::addContact(const QString &dn, const QString &sourceUid,
                                 const Contact::ContactSourceInfo &contactSourceInfo,
                                 const QString &name, const QString &company, const QString &mail,
                                 const QDateTime &lastModified,
                                 const QList<Contact::PhoneNumber> &phoneNumbers)
{

    const auto hid = hashifyCn(dn);

    bool newContactCreated = false;

    QMutexLocker lock(&m_feederMutex);

    Contact *contact = m_contacts.value(hid, nullptr);
    if (!contact) {
        newContactCreated = true;
        contact = new Contact(hid, dn, sourceUid, contactSourceInfo, name, this);
        m_contacts.insert(hid, contact);
        m_contactsBySourceId.insert(sourceUid, contact);
    }

    contact->setCompany(company);
    contact->setMail(mail);
    contact->setLastModified(lastModified);
    contact->addPhoneNumbers(phoneNumbers);

    if (!newContactCreated) {
        const auto &oldSourceInfo = contact->contactSourceInfo();
        if (oldSourceInfo != contactSourceInfo && contactSourceInfo.prio > oldSourceInfo.prio) {
            contact->setContactSourceInfo(contactSourceInfo);
        }
    }

    Q_EMIT contactAdded(contact);

    return contact;
}

void AddressBook::addContact(Contact *contact)
{
    QMutexLocker lock(&m_feederMutex);

    if (contact != nullptr && !m_contacts.contains(contact->id())) {
        contact->setParent(this);
        m_contacts.insert(contact->id(), contact);
        m_contactsBySourceId.insert(contact->sourceUid(), contact);

        Q_EMIT contactAdded(contact);
    }
}

Contact *AddressBook::modifyContact(const QString &dn, const QString &sourceUid,
                                    const QString &name, const QString &company,
                                    const QString &mail, const QDateTime &lastModified,
                                    const QList<Contact::PhoneNumber> &phoneNumbers)
{
    auto contact = lookupBySourceUid(sourceUid);

    QMutexLocker lock(&m_feederMutex);

    if (contact) {
        contact->setDisplayName(dn);
        contact->setName(name);
        contact->setCompany(company);
        contact->setMail(mail);
        contact->setLastModified(lastModified);
        contact->clearPhoneNumbers();
        contact->addPhoneNumbers(phoneNumbers);

        Q_EMIT contactModified(contact);

        return contact;
    }

    return nullptr;
}

void AddressBook::removeContact(const QString &sourceUid)
{
    auto contact = lookupBySourceUid(sourceUid);

    QMutexLocker lock(&m_feederMutex);

    if (contact) {
        m_contacts.remove(contact->id());
        m_contactsBySourceId.remove(contact->sourceUid());

        Q_EMIT contactRemoved(sourceUid);
    }
}

QHash<QString, Contact *> AddressBook::contacts() const
{
    return m_contacts;
}

void AddressBook::reserve(qsizetype size)
{
    m_contacts.reserve(size);
    m_contactsBySourceId.reserve(size);
}

QList<Contact *> AddressBook::search(const QString &searchString) const
{
    QList<Contact *> results;
    QList<qreal> weights;

    results.reserve(m_contacts.size());
    weights.reserve(m_contacts.size());

    const auto cleanSearchString = PhoneNumberUtil::clearInternationalChars(searchString);

    for (auto contact : std::as_const(m_contacts)) {
        const qreal weight = contact->matchesSearch(cleanSearchString);
        if (weight > 0.863) {
            weights.append(1.0 - weight);
            results.append(contact);
        }
    }

    FuzzyCompare::sortListByWeight(results, weights);

    return results;
}

Contact *AddressBook::lookupBySipUrl(const QString &sipUrl) const
{
    static const QRegularExpression sipNumberRegex("^.*sips?:(.*)@.*$",
                                                   QRegularExpression::CaseInsensitiveOption);

    const auto result = sipNumberRegex.match(sipUrl);
    if (result.hasMatch() && result.hasCaptured(1)) {
        return lookupByNumber(result.captured(1));
    }
    return nullptr;
}

Contact *AddressBook::lookupByNumber(const QString &number) const
{
    Contact *result = nullptr;

    for (auto contact : std::as_const(m_contacts)) {
        const auto &numbers = contact->phoneNumbers();
        for (const auto &phoneNumber : numbers) {
            if (phoneNumber.number == number) {
                if (result) {
                    // Already have a match (ambigous number) -> return no result to prevent the
                    // wrong contact taken as result
                    return nullptr;
                } else {
                    result = contact;
                }
            }
        }
    }

    return result;
}

Contact *AddressBook::lookupByContactId(const QString &contactId) const
{
    return m_contacts.value(contactId, nullptr);
}

Contact *AddressBook::lookupBySourceUid(const QString &sourceUid) const
{
    return m_contactsBySourceId.value(sourceUid, nullptr);
}

void AddressBook::clear()
{
    QMutexLocker lock(&m_feederMutex);

    qDeleteAll(m_contacts);
    m_contacts.clear();
    Q_EMIT contactsCleared();
}
