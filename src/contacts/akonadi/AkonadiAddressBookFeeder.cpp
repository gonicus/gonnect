#include <Akonadi/Session>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/EntityDisplayAttribute>
#include <KContacts/ContactGroup>
#include <QDebug>
#include <QLoggingCategory>

#include "AkonadiAddressBookFeeder.h"
#include "Contact.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "AvatarManager.h"
#include "ReadOnlyConfdSettings.h"

Q_LOGGING_CATEGORY(lcAkonadiAddressBookFeeder, "gonnect.app.feeder.AkonadiAddressBookFeeder")

AkonadiAddressBookFeeder::AkonadiAddressBookFeeder(const QString &group, AddressBookManager *parent)
    : QObject(parent),
      m_group(group),
      m_session(new Akonadi::Session("GOnnect::ContactsSession")),
      m_monitor(new Akonadi::Monitor(parent))
{
    ReadOnlyConfdSettings settings;

    settings.beginGroup(m_group);
    m_displayName = settings.value("displayName", "").toString();
    bool ok = true;
    m_priority = settings.value("prio", 0).toUInt(&ok);
    if (!ok) {
        qCWarning(lcAkonadiAddressBookFeeder) << "Could not parse priority value for" << m_group;
        m_priority = 0;
    }

    settings.endGroup();

    Akonadi::ItemFetchScope scope;
    scope.fetchFullPayload(true);

    m_monitor->setSession(m_session);
    m_monitor->fetchCollection(true);
    m_monitor->setItemFetchScope(scope);
    m_monitor->setCollectionMonitored(Akonadi::Collection::root());
    m_monitor->setMimeTypeMonitored(KContacts::Addressee::mimeType(), true);
    m_monitor->setMimeTypeMonitored(KContacts::ContactGroup::mimeType(), true);

    connect(m_monitor, &Akonadi::Monitor::itemAdded, this,
            [this](const Akonadi::Item &item, const Akonadi::Collection &) {
                if (item.mimeType() != "text/directory") {
                    return;
                }

                if (item.hasPayload<KContacts::Addressee>()) {
                    const KContacts::Addressee addressee = item.payload<KContacts::Addressee>();
                    auto &addressbook = AddressBook::instance();

                    QList<Contact::PhoneNumber> phoneNumbers;
                    QListIterator<KContacts::PhoneNumber> phoneNumberIterator(
                            addressee.phoneNumbers());
                    while (phoneNumberIterator.hasNext()) {
                        auto phoneNumber = phoneNumberIterator.next();

                        Contact::NumberType numberType = Contact::NumberType::Unknown;
                        if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Home) {
                            numberType = Contact::NumberType::Home;
                        } else if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Cell) {
                            numberType = Contact::NumberType::Mobile;
                        } else if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Work) {
                            numberType = Contact::NumberType::Commercial;
                        }

                        phoneNumbers.append({ numberType, phoneNumber.normalizedNumber(), false });
                    }

                    // Retrieve SIP URI's by reading the IMPP
                    // (Instant Messaging and Presence Protocol) field
                    QListIterator<KContacts::Impp> imppIterator(addressee.imppList());
                    while (imppIterator.hasNext()) {
                        auto imppEntry = imppIterator.next();

                        if (imppEntry.serviceType() == "sip") {
                            phoneNumbers.append({ Contact::NumberType::Unknown,
                                                  imppEntry.address().toString(), true });
                        }
                    }

                    QString email = "";
                    auto emails = addressee.emails();
                    if (emails.count()) {
                        email = emails.first();
                    }

                    QDateTime changed = QDateTime::currentDateTimeUtc();

                    Contact *contact = addressbook.addContact(
                            addressee.assembledName() + addressee.organization(), addressee.uid(),
                            { m_priority, m_displayName }, addressee.assembledName(),
                            addressee.organization(), email, changed, phoneNumbers);

                    if (!addressee.photo().isEmpty()) {
                        AvatarManager::instance().addExternalImage(
                                contact->id(), addressee.photo().rawData(), changed);
                    }
                }
            });

    connect(m_monitor, &Akonadi::Monitor::itemChanged, this,
            [](const Akonadi::Item &item, const QSet<QByteArray> &) {
                if (item.mimeType() != "text/directory") {
                    return;
                }

                if (item.hasPayload<KContacts::Addressee>()) {
                    const KContacts::Addressee addressee = item.payload<KContacts::Addressee>();
                    auto &addressbook = AddressBook::instance();

                    QList<Contact::PhoneNumber> phoneNumbers;
                    QListIterator<KContacts::PhoneNumber> phoneNumberIterator(
                            addressee.phoneNumbers());
                    while (phoneNumberIterator.hasNext()) {
                        auto phoneNumber = phoneNumberIterator.next();

                        Contact::NumberType numberType = Contact::NumberType::Unknown;
                        if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Home) {
                            numberType = Contact::NumberType::Home;
                        } else if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Cell) {
                            numberType = Contact::NumberType::Mobile;
                        } else if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Work) {
                            numberType = Contact::NumberType::Commercial;
                        }

                        phoneNumbers.append({ numberType, phoneNumber.normalizedNumber(), false });
                    }

                    QListIterator<KContacts::Impp> imppIterator(addressee.imppList());
                    while (imppIterator.hasNext()) {
                        auto imppEntry = imppIterator.next();

                        if (imppEntry.serviceType() == "sip") {
                            phoneNumbers.append({ Contact::NumberType::Unknown,
                                                  imppEntry.address().toString(), true });
                        }
                    }

                    QString email = "";
                    auto emails = addressee.emails();
                    if (emails.count()) {
                        email = emails.first();
                    }

                    QDateTime changed = addressee.revision();

                    Contact *contact = addressbook.modifyContact(
                            addressee.assembledName() + addressee.organization(), addressee.uid(),
                            addressee.assembledName(), addressee.organization(), email, changed,
                            phoneNumbers);

                    if (contact && !addressee.photo().isEmpty()) {
                        AvatarManager::instance().addExternalImage(
                                contact->id(), addressee.photo().rawData(), changed);
                    }
                }
            });

    connect(m_monitor, &Akonadi::Monitor::itemRemoved, this, [](const Akonadi::Item &item) {
        if (item.mimeType() != "text/directory") {
            return;
        }

        if (item.hasPayload<KContacts::Addressee>()) {
            const KContacts::Addressee addressee = item.payload<KContacts::Addressee>();
            auto &addressbook = AddressBook::instance();

            QString uid = addressee.uid();
            if (auto contact = AddressBook::instance().lookupBySourceUid(uid)) {
                AvatarManager::instance().removeExternalImage(contact->id());
            }
            addressbook.removeContact(uid);
        }
    });
}

void AkonadiAddressBookFeeder::process()
{
    m_job = new Akonadi::ContactSearchJob();
    connect(m_job, &Akonadi::ContactSearchJob::result, this,
            [this](KJob *job) { processSearchResult(job); });

    m_job->setAutoDelete(true);
    m_job->start();
}

void AkonadiAddressBookFeeder::processSearchResult(KJob *job)
{
    auto &addressbook = AddressBook::instance();

    Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob *>(job);
    const KContacts::Addressee::List contacts = searchJob->contacts();

    qCDebug(lcAkonadiAddressBookFeeder) << "received" << contacts.length() << "contacts";

    QListIterator<KContacts::Addressee> it(contacts);
    while (it.hasNext()) {
        auto addressee = it.next();

        QList<Contact::PhoneNumber> phoneNumbers;
        QListIterator<KContacts::PhoneNumber> phoneNumberIterator(addressee.phoneNumbers());
        while (phoneNumberIterator.hasNext()) {
            auto phoneNumber = phoneNumberIterator.next();

            Contact::NumberType numberType = Contact::NumberType::Unknown;
            if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Home) {
                numberType = Contact::NumberType::Home;
            } else if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Cell) {
                numberType = Contact::NumberType::Mobile;
            } else if (phoneNumber.type() == KContacts::PhoneNumber::TypeFlag::Work) {
                numberType = Contact::NumberType::Commercial;
            }

            phoneNumbers.append({ numberType, phoneNumber.normalizedNumber(), false });
        }

        QListIterator<KContacts::Impp> imppIterator(addressee.imppList());
        while (imppIterator.hasNext()) {
            auto imppEntry = imppIterator.next();

            if (imppEntry.serviceType() == "sip") {
                phoneNumbers.append(
                        { Contact::NumberType::Unknown, imppEntry.address().toString(), true });
            }
        }

        QString email = "";
        auto emails = addressee.emails();
        if (emails.count()) {
            email = emails.first();
        }

        QDateTime changed = QDateTime::currentDateTimeUtc();

        Contact *contact = addressbook.addContact(
                addressee.assembledName() + addressee.organization(), addressee.uid(),
                { m_priority, m_displayName }, addressee.assembledName(), addressee.organization(),
                email, changed, phoneNumbers);

        if (!addressee.photo().isEmpty()) {
            AvatarManager::instance().addExternalImage(contact->id(), addressee.photo().rawData(),
                                                       changed);
        }
    }

    Q_EMIT addressbook.contactsReady();
}

AkonadiAddressBookFeeder::~AkonadiAddressBookFeeder()
{
    delete m_monitor;
    delete m_session;
}
