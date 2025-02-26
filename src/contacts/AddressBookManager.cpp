#include "AddressBookManager.h"
#include "ReadOnlyConfdSettings.h"
#include "NetworkHelper.h"
#include "AddressBook.h"
#include "LDAPAddressBookFeeder.h"
#include "CardDAVAddressBookFeeder.h"
#include "CsvFileAddressBookFeeder.h"
#include "AvatarManager.h"
#include "ViewHelper.h"
#include "SecretPortal.h"
#include "KeychainSettings.h"

#include <QTimer>
#include <QUrl>
#include <QRegularExpression>
#include <QLoggingCategory>
#include <QCryptographicHash>

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

Q_LOGGING_CATEGORY(lcAddressBookManager, "gonnect.app.addressbook")

AddressBookManager::AddressBookManager(QObject *parent) : QObject{ parent } { }

QString AddressBookManager::secret(const QString &group) const
{
    KeychainSettings keychainSettings;
    keychainSettings.beginGroup(group);
    return keychainSettings.value("secret", "").toString();
}

QString AddressBookManager::hashForSettingsGroup(const QString &group) const
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(group);

    QString groupSettingsStr;
    auto childKeys = settings.childKeys();
    std::sort(childKeys.begin(), childKeys.end());

    for (const auto &key : childKeys) {
        groupSettingsStr.append(key);
        groupSettingsStr.append(settings.value(key, "").toString());
    }

    settings.endGroup();

    return QCryptographicHash::hash(groupSettingsStr.toUtf8(), QCryptographicHash::Md5).toHex();
}

void AddressBookManager::initAddressBookConfigs()
{
    static QRegularExpression ldapGroupRegex = QRegularExpression("^ldap[0-9]+$");
    static QRegularExpression csvGroupRegex = QRegularExpression("^csv[0-9]+$");
    static QRegularExpression carddavGroupRegex = QRegularExpression("^carddav[0-9]+$");

    ReadOnlyConfdSettings settings;
    const QStringList groups = settings.childGroups();

    for (const auto &group : groups) {
        if (ldapGroupRegex.match(group).hasMatch() || csvGroupRegex.match(group).hasMatch()
            || carddavGroupRegex.match(group).hasMatch()) {
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
    static QRegularExpression carddavGroupRegex = QRegularExpression("^carddav[0-9]+$");

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
        } else if (carddavGroupRegex.match(group).hasMatch()) {
            if (processCardDAVAddressBookConfig(group)) {
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
    const auto groupHash = hashForSettingsGroup(group);
    const auto secretKey = QString("%1_%2").arg(group, groupHash);
    const QString secret = this->secret(secretKey);

    ReadOnlyConfdSettings settings;
    settings.beginGroup(group);
    const auto bindMethodStr = settings.value("bindMethod", "none").toString();

    if (bindMethodStr == "simple" && secret.isEmpty()) {
        auto &viewHelper = ViewHelper::instance();
        auto conn = connect(&viewHelper, &ViewHelper::ldapPasswordResponded, this,
                            [group, secretKey, this](const QString &id, const QString &password) {
                                if (id == group) {
                                    QObject::disconnect(m_viewHelperConnections.value(group));
                                    m_viewHelperConnections.remove(group);

                                    auto &secretPortal = SecretPortal::instance();
                                    if (secretPortal.isValid()) {
                                        KeychainSettings settings;
                                        settings.beginGroup(secretKey);
                                        const auto secret = secretPortal.encrypt(password);
                                        settings.setValue("secret", secret);
                                        settings.endGroup();
                                    }

                                    processLDAPAddressBookConfigImpl(group, password);
                                }
                            });

        m_viewHelperConnections.insert(group, conn);
        viewHelper.requestLdapPassword(group, settings.value("host", "").toString());

    } else {
        auto &secretPortal = SecretPortal::instance();
        if (secretPortal.isValid()) {
            if (secretPortal.isInitialized()) {
                processLDAPAddressBookConfigImpl(group, secretPortal.decrypt(secret));
            } else {
                connect(&secretPortal, &SecretPortal::initializedChanged, this,
                        [this, group, secret]() {
                            processLDAPAddressBookConfigImpl(
                                    group, SecretPortal::instance().decrypt(secret));
                        });
            }
        }
    }

    return true;
}

bool AddressBookManager::processLDAPAddressBookConfigImpl(const QString &group,
                                                          const QString &password)
{
    auto &nh = NetworkHelper::instance();
    ReadOnlyConfdSettings settings;

    QString url = settings.value(group + "/url", "").toString();
    if (nh.isReachable(url)) {
        settings.beginGroup(group);
        const auto scriptableAttributes =
                settings.value("sipStatusSubscriptableAttributes", "").toString();

        const auto bindMethodStr = settings.value("bindMethod", "none").toString();
        LDAPAddressBookFeeder::BindMethod bindMethod;

        static const QHash<QString, LDAPAddressBookFeeder::BindMethod> s_bindMethods = {
            { "none", LDAPAddressBookFeeder::BindMethod::None },
            { "simple", LDAPAddressBookFeeder::BindMethod::Simple },
            { "gssapi", LDAPAddressBookFeeder::BindMethod::GSSAPI },
        };

        if (s_bindMethods.contains(bindMethodStr)) {
            bindMethod = s_bindMethods.value(bindMethodStr);
        } else {
            qCCritical(lcAddressBookManager).nospace()
                    << "Unknown LDAP bind method '" << bindMethodStr
                    << "' - initialization of LDAP account will be aborted.";
            return false;
        }

        LDAPAddressBookFeeder feeder(
                settings.value("useSSL", true).toBool(),
                url, settings.value("base", "").toString(), settings.value("filter", "").toString(),
                bindMethod, settings.value("bindDn", "").toString(), password,
                settings.value("realm", "").toString(), settings.value("authcid", "").toString(),
                settings.value("authzid", "").toString(),
                settings.value("caFile", "").toString(),
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

bool AddressBookManager::processCardDAVAddressBookConfig(const QString &group)
{
    const auto groupHash = hashForSettingsGroup(group);
    const auto secretKey = QString("%1_%2").arg(group, groupHash);
    const QString secret = this->secret(secretKey);

    if (secret.isEmpty()) {
        auto &viewHelper = ViewHelper::instance();
        auto conn = connect(&viewHelper, &ViewHelper::cardDavPasswordResponded, this,
                            [group, secretKey, this](const QString &id, const QString &password) {
                                if (id == group) {
                                    QObject::disconnect(m_viewHelperConnections.value(group));
                                    m_viewHelperConnections.remove(group);

                                    auto &secretPortal = SecretPortal::instance();
                                    if (secretPortal.isValid()) {
                                        KeychainSettings settings;
                                        settings.beginGroup(secretKey);
                                        const auto secret = secretPortal.encrypt(password);
                                        settings.setValue("secret", secret);
                                        settings.endGroup();
                                    }

                                    processCardDAVAddressBookConfigImpl(group, password);
                                }
                            });

        m_viewHelperConnections.insert(group, conn);

        ReadOnlyConfdSettings settings;
        settings.beginGroup(group);
        viewHelper.requestCardDavPassword(group, settings.value("host", "").toString());
        settings.endGroup();

    } else {
        auto &secretPortal = SecretPortal::instance();
        if (secretPortal.isValid()) {
            if (secretPortal.isInitialized()) {
                processCardDAVAddressBookConfigImpl(group, secretPortal.decrypt(secret));
            } else {
                connect(&secretPortal, &SecretPortal::initializedChanged, this,
                        [this, group, secret]() {
                            processCardDAVAddressBookConfigImpl(
                                    group, SecretPortal::instance().decrypt(secret));
                        });
            }
        }
    }


    return true;
}

void AddressBookManager::processCardDAVAddressBookConfigImpl(const QString &group,
                                                             const QString &password)
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(group);

    QHash<QString, QString> settingsHash;
    const auto keys = settings.allKeys();
    for (const auto &key : keys) {
        qCritical() << key << settings.value(key);
        settingsHash.insert(key, settings.value(key, "").toString());
    }
    const auto controlHash = qHash(settingsHash);

    const bool useSSL = settings.value("useSSL", true).toBool();

    auto feeder = new CardDAVAddressBookFeeder(
            controlHash, settings.value("host", "").toString(),
            settings.value("path", "").toString(), settings.value("user", "").toString(), password,
            settings.value("port", useSSL ? 443 : 80).toInt(), useSSL, this);
    feeder->feedAddressBook(AddressBook::instance());
    Q_UNUSED(feeder)
    settings.endGroup();
}
