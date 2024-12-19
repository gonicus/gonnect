#include "AddressBookManager.h"
#include "ReadOnlyConfdSettings.h"
#include "NetworkHelper.h"
#include "AddressBook.h"
#include "LDAPAddressBookFeeder.h"
#include "CsvFileAddressBookFeeder.h"
#include "AvatarManager.h"

#include <QTimer>
#include <QUrl>
#include <QRegularExpression>
#include <QLoggingCategory>

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

Q_LOGGING_CATEGORY(lcAddressBookManager, "gonnect.app.addressbook")

AddressBookManager::AddressBookManager(QObject *parent) : QObject{ parent } { }

void AddressBookManager::initAddressBookConfigs()
{
    static QRegularExpression ldapGroupRegex = QRegularExpression("^ldap[0-9]+$");
    static QRegularExpression csvGroupRegex = QRegularExpression("^csv[0-9]+$");

    ReadOnlyConfdSettings settings;
    const QStringList groups = settings.childGroups();

    for (const auto &group : groups) {
        if (ldapGroupRegex.match(group).hasMatch() || csvGroupRegex.match(group).hasMatch()) {
            m_addressBookConfigs.push_back(group);
        }
    }
}

void AddressBookManager::reloadAddressBook()
{
    AddressBook::instance().clear();
    m_addressBookQueue = m_addressBookConfigs;
    processAddressBookQueue();
}

void AddressBookManager::processAddressBookQueue()
{
    static QRegularExpression ldapGroupRegex = QRegularExpression("^ldap[0-9]+$");
    static QRegularExpression csvGroupRegex = QRegularExpression("^csv[0-9]+$");

    QMutableStringListIterator it(m_addressBookQueue);
    while (it.hasNext()) {
        QString group = it.next();

        if (ldapGroupRegex.match(group).hasMatch()) {
            if (processLDAPAddressBookConfig(group)) {
                it.remove();
            }
        } else if (csvGroupRegex.match(group).hasMatch()) {
            if (processCSVAddressBookConfig(group)) {
                it.remove();
            }
        }
    }

    if (!m_addressBookQueue.isEmpty()) {
        qCCritical(lcAddressBookManager) << "Queue not empty - retrying in 30s";
        QTimer::singleShot(30s, this, &AddressBookManager::processAddressBookQueue);
    } else {
        emit AddressBook::instance().contactsReady();
    }
}

bool AddressBookManager::processLDAPAddressBookConfig(const QString &group)
{
    auto &nh = NetworkHelper::instance();
    ReadOnlyConfdSettings settings;

    QString url = settings.value(group + "/url", "").toString();
    if (nh.isReachable(url)) {
        settings.beginGroup(group);
        const auto scriptableAttributes =
                settings.value("sipStatusSubscriptableAttributes", "").toString();

        LDAPAddressBookFeeder feeder(
                url, settings.value("base", "").toString(), settings.value("filter", "").toString(),
                scriptableAttributes.isEmpty() ? QStringList()
                                               : scriptableAttributes.split(QChar(',')),
                settings.value("baseNumber", "").toString());

        feeder.feedAddressBook(AddressBook::instance());

        AvatarManager::instance().initialLoad(url, settings.value("base", "").toString(),
                                              settings.value("filter", "").toString());

        settings.endGroup();

        return true;
    }

    qCWarning(lcAddressBookManager).nospace()
            << "Failed to load LDAP source " << qPrintable(url) << ": not reachable";
    return false;
}

bool AddressBookManager::processCSVAddressBookConfig(const QString &group)
{
    ReadOnlyConfdSettings settings;

    settings.beginGroup(group);
    CsvFileAddressBookFeeder feeder(settings.value("path", "").toString());
    feeder.feedAddressBook(AddressBook::instance());
    settings.endGroup();

    return true;
}
