#include "AddressBook.h"
#include "Contact.h"
#include "FuzzyCompare.h"
#include "PhoneNumberUtil.h"

#include <QCryptographicHash>
#include <QRegularExpression>
#include <QLoggingCategory>

#ifndef APP_TESTS
#  include "AvatarPrioHelper.h"
#endif

Q_LOGGING_CATEGORY(lcAddressBook, "gonnect.app.contacts.AddressBook")

AddressBook::AddressBook(QObject *parent) : QObject{ parent }
{
    connect(this, &AddressBook::contactAdded, this, &AddressBook::updateSourceInfos);
    connect(this, &AddressBook::contactsCleared, this, [this]() {
        if (m_contactSourceInfos.size()) {
            m_contactSourceInfos.clear();
            Q_EMIT contactSourceInfosChanged();
        }
    });

#ifndef APP_TESTS
    connect(&AvatarPrioHelper::instance(), &AvatarPrioHelper::priosChanged, this, [this]() {
        const auto contacts = m_contacts.values();
        for (auto *contact : std::as_const(contacts)) {
            contact->updateAvatar();
        }
    });
#endif
}

#ifndef APP_TESTS
void AddressBook::initContactSignals(Contact *contact)
{
    if (!contact) {
        qCCritical(lcAddressBook) << "contact may not be nullptr";
        return;
    }

    connect(contact, &Contact::chatUserAdded, this, [this, contact](ChatUser *chatUser) {
        Q_CHECK_PTR(chatUser);
        addChatUserMapping(chatUser, contact);
    });
    connect(contact, &Contact::chatUserRemoved, this, [this](const ChatUser *chatUser) {
        Q_CHECK_PTR(chatUser);
        removeChatUserMapping(chatUser);
    });
    connect(contact, &Contact::avatarChanged, this, [this, contact]() {
        const auto chatUsers = contact->chatUsers();
        for (auto *chatUser : std::as_const(chatUsers)) {
            Q_EMIT chatUserAvatarChanged(chatUser);
        }
    });
}

void AddressBook::addChatUserMapping(ChatUser *chatUser, Contact *contact)
{
    if (!contact) {
        qCCritical(lcAddressBook) << "contact may not be nullptr";
        return;
    }
    if (!chatUser) {
        qCCritical(lcAddressBook) << "chatUser may not be nullptr";
        return;
    }

    m_contactsByChatUser.insert(chatUser, contact);
    Q_EMIT chatUserMappingAdded(chatUser, contact);
}

void AddressBook::removeChatUserMapping(const ChatUser *chatUser)
{
    if (!chatUser) {
        qCCritical(lcAddressBook) << "chatUser may not be nullptr";
        return;
    }

    m_contactsByChatUser.remove(chatUser);
}
#endif

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
                                 const QList<Contact::PhoneNumber> &phoneNumbers,
                                 BlockInfo blockInfo)
{

    const auto hid = hashifyCn(dn);

    bool newContactCreated = false;

    QMutexLocker lock(&m_feederMutex);

    Contact *contact = m_contacts.value(hid, nullptr);
    if (!contact) {
        newContactCreated = true;
        contact = new Contact(hid, dn, sourceUid, contactSourceInfo, name, blockInfo, this);
        m_contacts.insert(hid, contact);
        m_contactsBySourceId.insert(sourceUid, contact);
#ifndef APP_TESTS
        initContactSignals(contact);
#endif
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
#ifndef APP_TESTS
        initContactSignals(contact);
#endif

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
        const auto contactId = contact->id();
        contact->disconnect(this);
        m_contacts.remove(contactId);
        m_contactsBySourceId.remove(contact->sourceUid());

#ifndef APP_TESTS
        const auto &chatUsers = contact->chatUsers();
        for (const auto *chatUser : chatUsers) {
            removeChatUserMapping(chatUser);
        }
#endif

        Q_EMIT contactRemoved(contactId);
    }
}

void AddressBook::resetContacts()
{
    QMutexLocker lock(&m_feederMutex);

    qDeleteAll(m_contacts);
    m_contacts.clear();
    m_contactsBySourceId.clear();
    m_contactsByChatUser.clear();
    Q_EMIT contactsCleared();
}

void AddressBook::removeContactsBySource(const QString &source)
{
    QMutexLocker lock(&m_feederMutex);

    QString sourceUid;
    bool sourceInfoCleared = false;

    QMutableHashIterator it(m_contacts);
    while (it.hasNext()) {
        it.next();

        const auto contact = it.value();
        if (contact->contactSourceInfo().configId == source) {
            sourceUid = contact->sourceUid();

            // Remove the ContactSourceInfo of the contact source
            if (!sourceInfoCleared) {
                m_contactSourceInfos.removeAll(contact->contactSourceInfo());
                sourceInfoCleared = true;

                Q_EMIT contactSourceInfosChanged();
            }

            const auto contactId = contact->id();
            contact->disconnect(this);
            m_contactsBySourceId.remove(sourceUid);

#ifndef APP_TESTS
            const auto &chatUsers = contact->chatUsers();
            for (const auto *chatUser : chatUsers) {
                removeChatUserMapping(chatUser);
            }
#endif

            it.remove();
            Q_EMIT contactRemoved(contactId);
        }
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

QList<Contact *> AddressBook::search(const QString &searchString, bool includeBlocked) const
{
    QList<Contact *> results;
    QList<qreal> weights;

    results.reserve(m_contacts.size());
    weights.reserve(m_contacts.size());

    const auto cleanSearchString = PhoneNumberUtil::clearInternationalChars(searchString);

    for (auto contact : std::as_const(m_contacts)) {
        if (!includeBlocked && contact->blockInfo().isBlocking) {
            continue;
        }

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
    unsigned bestPrio = 0;

    for (auto contact : std::as_const(m_contacts)) {
        const auto &numbers = contact->phoneNumbers();
        for (const auto &phoneNumber : numbers) {
            if (phoneNumber.number == number) {
                const unsigned currPrio = contact->contactSourceInfo().prio;
                if (!result || currPrio > bestPrio
                    || (currPrio == bestPrio
                        && contact->contactSourceInfo().displayName.localeAwareCompare(
                                   result->contactSourceInfo().displayName)
                                < 0)) {
                    result = contact;
                    bestPrio = currPrio;
                }
                break;
            }
        }
    }

    return result;
}

Contact *AddressBook::lookupByEmail(const QString &emailAddr) const
{
    for (auto contact : std::as_const(m_contacts)) {
        if (contact->mail() == emailAddr) {
            return contact;
        }
    }
    return nullptr;
}

Contact *AddressBook::lookupByContactId(const QString &contactId) const
{
    return m_contacts.value(contactId, nullptr);
}

Contact *AddressBook::lookupBySourceUid(const QString &sourceUid) const
{
    return m_contactsBySourceId.value(sourceUid, nullptr);
}

#ifndef APP_TESTS
Contact *AddressBook::lookupByChatUser(const ChatUser *chatUser) const
{
    return chatUser ? m_contactsByChatUser.value(chatUser) : nullptr;
}
#endif
