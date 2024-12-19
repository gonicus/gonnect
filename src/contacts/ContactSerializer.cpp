#include "ContactSerializer.h"
#include "AddressBook.h"
#include "Contact.h"

#include <QDebug>
#include <QFile>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcContactSerializer, "gonnect.app.ContactSerializer")

#define GONNECT_MODEL_MAGIC 0x1337
#define GONNECT_MODEL_VERSION 0x01

ContactSerializer::ContactSerializer(const QString &filePath, QObject *parent)
    : QObject{ parent }, m_filePath{ filePath }
{
}

void ContactSerializer::saveAddressBook(const AddressBook &addressBook)
{
    qCInfo(lcContactSerializer) << "Writing address book to file" << m_filePath;

    QFile file(m_filePath);

    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);

        out << static_cast<quint16>(GONNECT_MODEL_MAGIC);
        out << static_cast<quint8>(GONNECT_MODEL_VERSION);

        const auto contacts = addressBook.contacts().values();
        out << contacts.size();
        for (const auto contact : contacts) {
            out << *contact;
        }

    } else {
        qCCritical(lcContactSerializer)
                << QString("Error: file %1 could not be opened/created for writing: %2 (code %3)")
                           .arg(m_filePath, file.errorString())
                           .arg(file.error());
    }
}

void ContactSerializer::loadAddressBook(AddressBook &addressBook)
{
    qCInfo(lcContactSerializer) << "Reading address book from file" << m_filePath;

    QFile file(m_filePath);

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);

        quint16 magic;
        quint8 version;
        qsizetype numberOfContacts;

        in >> magic;
        in >> version;
        in >> numberOfContacts;

        if (magic != GONNECT_MODEL_MAGIC) {
            qCCritical(lcContactSerializer) << "Error: Unkown file type - aborting";
            return;
        }
        if (version != GONNECT_MODEL_VERSION) {
            qCCritical(lcContactSerializer)
                    << QString("Error: Version mismatch (expecting %1 but file is %2) - aborting")
                               .arg(GONNECT_MODEL_VERSION)
                               .arg(version);
            return;
        }

        addressBook.reserve(numberOfContacts);

        for (qsizetype i = 0; i < numberOfContacts; ++i) {
            QString id;
            QString dn;
            QString name;
            QString company;
            QString mail;
            QDateTime lastModified;
            bool sipStatusSubscriptable;
            QList<Contact::PhoneNumber> phoneNumbers;

            in >> id;
            in >> dn;
            in >> name;
            in >> company;
            in >> mail;
            in >> lastModified;
            in >> sipStatusSubscriptable;
            in >> phoneNumbers;

            Contact *contact = new Contact(id, dn, name, company, mail, lastModified, phoneNumbers,
                                           &addressBook);
            addressBook.addContact(contact);
        }

    } else {
        qCCritical(lcContactSerializer)
                << QString("Error: file %1 could not be opened for reading: %2 (code %3)")
                           .arg(m_filePath, file.errorString())
                           .arg(file.error());
    }
}

#undef GONNECT_MODEL_MAGIC
#undef GONNECT_MODEL_VERSION
