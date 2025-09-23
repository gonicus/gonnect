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
                    const KContacts::Addressee addr = item.payload<KContacts::Addressee>();
                    auto &addressbook = AddressBook::instance();

                    QList<Contact::PhoneNumber> phoneNumbers;
                    QListIterator<KContacts::PhoneNumber> pit(addr.phoneNumbers());
                    while (pit.hasNext()) {
                        auto pn = pit.next();

                        Contact::NumberType nt = Contact::NumberType::Unknown;
                        if (pn.type() == KContacts::PhoneNumber::TypeFlag::Home) {
                            nt = Contact::NumberType::Home;
                        } else if (pn.type() == KContacts::PhoneNumber::TypeFlag::Cell) {
                            nt = Contact::NumberType::Mobile;
                        } else if (pn.type() == KContacts::PhoneNumber::TypeFlag::Work) {
                            nt = Contact::NumberType::Commercial;
                        }

                        phoneNumbers.append({ nt, pn.normalizedNumber(), false });
                    }

                    QListIterator<KContacts::Impp> imit(addr.imppList());
                    while (imit.hasNext()) {
                        auto imn = imit.next();

                        if (imn.serviceType() == "sip") {
                            phoneNumbers.append({ Contact::NumberType::Unknown,
                                                  imn.address().toString(), true });
                        }
                    }

                    QString email = "";
                    auto emails = addr.emails();
                    if (emails.count()) {
                        email = emails.first();
                    }

                    QDateTime changed = QDateTime::currentDateTimeUtc();

                    Contact *contact = addressbook.addContact(
                            addr.assembledName() + addr.organization(), addr.uid(),
                            { m_priority, m_displayName }, addr.assembledName(),
                            addr.organization(), email, changed, phoneNumbers);

                    if (!addr.photo().isEmpty()) {
                        AvatarManager::instance().addExternalImage(contact->id(),
                                                                   addr.photo().rawData(), changed);
                    }
                }
            });

    connect(m_monitor, &Akonadi::Monitor::itemChanged, this,
            [](const Akonadi::Item &item, const QSet<QByteArray> &) {
                if (item.mimeType() != "text/directory") {
                    return;
                }

                if (item.hasPayload<KContacts::Addressee>()) {
                    const KContacts::Addressee addr = item.payload<KContacts::Addressee>();
                    auto &addressbook = AddressBook::instance();

                    QList<Contact::PhoneNumber> phoneNumbers;
                    QListIterator<KContacts::PhoneNumber> pit(addr.phoneNumbers());
                    while (pit.hasNext()) {
                        auto pn = pit.next();

                        Contact::NumberType nt = Contact::NumberType::Unknown;
                        if (pn.type() == KContacts::PhoneNumber::TypeFlag::Home) {
                            nt = Contact::NumberType::Home;
                        } else if (pn.type() == KContacts::PhoneNumber::TypeFlag::Cell) {
                            nt = Contact::NumberType::Mobile;
                        } else if (pn.type() == KContacts::PhoneNumber::TypeFlag::Work) {
                            nt = Contact::NumberType::Commercial;
                        }

                        phoneNumbers.append({ nt, pn.normalizedNumber(), false });
                    }

                    QListIterator<KContacts::Impp> imit(addr.imppList());
                    while (imit.hasNext()) {
                        auto imn = imit.next();

                        if (imn.serviceType() == "sip") {
                            phoneNumbers.append({ Contact::NumberType::Unknown,
                                                  imn.address().toString(), true });
                        }
                    }

                    QString email = "";
                    auto emails = addr.emails();
                    if (emails.count()) {
                        email = emails.first();
                    }

                    QDateTime changed = addr.revision();
                    qCritical() << "####" << addr.revision();

                    Contact *contact = addressbook.modifyContact(
                            addr.assembledName() + addr.organization(), addr.uid(),
                            addr.assembledName(), addr.organization(), email, changed,
                            phoneNumbers);

                    if (contact && !addr.photo().isEmpty()) {
                        AvatarManager::instance().addExternalImage(contact->id(),
                                                                   addr.photo().rawData(), changed);
                    }
                }
            });

    connect(m_monitor, &Akonadi::Monitor::itemRemoved, this, [](const Akonadi::Item &item) {
        if (item.mimeType() != "text/directory") {
            return;
        }

        if (item.hasPayload<KContacts::Addressee>()) {
            const KContacts::Addressee addr = item.payload<KContacts::Addressee>();
            auto &addressbook = AddressBook::instance();

            QString uid = addr.uid();
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
        QListIterator<KContacts::PhoneNumber> pit(addressee.phoneNumbers());
        while (pit.hasNext()) {
            auto pn = pit.next();

            Contact::NumberType nt = Contact::NumberType::Unknown;
            if (pn.type() == KContacts::PhoneNumber::TypeFlag::Home) {
                nt = Contact::NumberType::Home;
            } else if (pn.type() == KContacts::PhoneNumber::TypeFlag::Cell) {
                nt = Contact::NumberType::Mobile;
            } else if (pn.type() == KContacts::PhoneNumber::TypeFlag::Work) {
                nt = Contact::NumberType::Commercial;
            }

            phoneNumbers.append({ nt, pn.normalizedNumber(), false });
        }

        QListIterator<KContacts::Impp> imit(addressee.imppList());
        while (imit.hasNext()) {
            auto imn = imit.next();

            imn.address().scheme();
            if (imn.serviceType() == "sip") {
                phoneNumbers.append(
                        { Contact::NumberType::Unknown, imn.address().toString(), true });
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
