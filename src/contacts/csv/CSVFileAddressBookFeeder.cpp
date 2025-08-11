#include "CSVFileAddressBookFeeder.h"
#include "Contact.h"
#include "AddressBook.h"
#include "ReadOnlyConfdSettings.h"
#include "AddressBookManager.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QFile>

Q_LOGGING_CATEGORY(lcCsvAddressBookFeeder, "gonnect.app.feeder.CsvAddressBookFeeder")

CsvFileAddressBookFeeder::CsvFileAddressBookFeeder(const QString &group, AddressBookManager *parent)
    : QObject(parent), m_group(group)
{
}

void CsvFileAddressBookFeeder::process()
{
    ReadOnlyConfdSettings settings;

    settings.beginGroup(m_group);
    m_filePath = settings.value("path", "").toString();
    m_displayName = settings.value("displayName", "").toString();

    bool ok = true;
    m_priority = settings.value("prio", 0).toUInt(&ok);
    if (!ok) {
        qCWarning(lcCsvAddressBookFeeder) << "Could not parse priority value for" << m_group;
        m_priority = 0;
    }

    feedAddressBook();
    settings.endGroup();
}

void CsvFileAddressBookFeeder::feedAddressBook()
{
    auto &addressbook = AddressBook::instance();

    QFile file(m_filePath);
    if (!file.exists()) {
        qCCritical(lcCsvAddressBookFeeder)
                << "File path" << m_filePath << "does not exist - aborting";
        return;
    }

    qCInfo(lcCsvAddressBookFeeder) << "Loading csv file" << m_filePath;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCCritical(lcCsvAddressBookFeeder) << "Unable to open file" << m_filePath << "- aborting";
        return;
    }

    quint32 contactCount = 0;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith(QChar('#'))) { // Empty lines and comments
            continue;
        }

        const auto splitted = line.split(QChar(';'));
        if (splitted.length() != 8) {
            qCCritical(lcCsvAddressBookFeeder)
                    << "Expected 8 elements in line, but found" << splitted.length() << "at line"
                    << line << "- aborting";
            return;
        }

        // Create contact out of line
        QList<Contact::PhoneNumber> phoneNumbers;
        if (!splitted.at(2).isEmpty()) {
            phoneNumbers.append(
                    { Contact::NumberType::Commercial, splitted.at(2), splitted.at(3) == "true" });
        }
        if (!splitted.at(4).isEmpty()) {
            phoneNumbers.append(
                    { Contact::NumberType::Mobile, splitted.at(4), splitted.at(5) == "true" });
        }
        if (!splitted.at(6).isEmpty()) {
            phoneNumbers.append(
                    { Contact::NumberType::Home, splitted.at(6), splitted.at(7) == "true" });
        }

        addressbook.addContact(splitted.at(0) + splitted.at(1), "", { m_priority, m_displayName },
                               splitted.at(0), splitted.at(1), "", QDateTime(), phoneNumbers);
        ++contactCount;
    }

    qCInfo(lcCsvAddressBookFeeder) << "Finished processing csv file" << m_filePath << "loaded"
                                   << contactCount << "contacts";
}
