#include "AddressBook.h"
#include "Contact.h"
#include "FuzzyCompare.h"
#include "PhoneNumberUtil.h"

#include <QCryptographicHash>
#include <QRegularExpression>

AddressBook::AddressBook(QObject *parent) : QObject{ parent } { }

QString AddressBook::hashifyCn(const QString &cn) const
{
    return QCryptographicHash::hash(cn.toUtf8(), QCryptographicHash::Sha256).toHex();
}

Contact *AddressBook::addContact(const QString &dn, const QString &name, const QString &company,
                                 const QString &mail, const QDateTime &lastModified,
                                 const QList<Contact::PhoneNumber> &phoneNumbers)
{

    const auto hid = hashifyCn(dn);

    Contact *contact = m_contacts.value(hid, nullptr);
    if (!contact) {
        contact = new Contact(hid, dn, name, this);
        m_contacts.insert(hid, contact);
    }

    contact->setCompany(company);
    contact->setMail(mail);
    contact->setLastModified(lastModified);
    contact->addPhoneNumbers(phoneNumbers);

    return contact;
}

void AddressBook::addContact(Contact *contact)
{
    if (contact != nullptr && !m_contacts.contains(contact->id())) {
        contact->setParent(this);
        m_contacts.insert(contact->id(), contact);
    }
}

QHash<QString, Contact *> AddressBook::contacts() const
{
    return m_contacts;
}

void AddressBook::reserve(qsizetype size)
{
    m_contacts.reserve(size);
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

void AddressBook::clear()
{
    qDeleteAll(m_contacts);
    m_contacts.clear();
    emit contactsCleared();
}
