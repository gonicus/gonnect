#include "AddressBookManager.h"
#include "ReadOnlyConfdSettings.h"
#include "AddressBook.h"
#include "KeychainSettings.h"
#include "IAddressBookFactory.h"
#include "ViewHelper.h"
#include "NetworkHelper.h"
#include "Credentials.h"

#include <QTimer>
#include <QUrl>
#include <QRegularExpression>
#include <QLoggingCategory>
#include <QCryptographicHash>
#include <QPluginLoader>

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

QString AddressBookManager::hashForSettingsGroup(const QString &group)
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(group);

    QString groupSettingsStr;
    auto childKeys = settings.childKeys();
    std::sort(childKeys.begin(), childKeys.end());

    for (const auto &key : std::as_const(childKeys)) {
        groupSettingsStr.append(key);
        groupSettingsStr.append(settings.value(key, "").toString());
    }

    settings.endGroup();

    return QCryptographicHash::hash(groupSettingsStr.toUtf8(), QCryptographicHash::Md5).toHex();
}

void AddressBookManager::initAddressBookConfigs()
{
    ReadOnlyConfdSettings settings;

    const QObjectList &staticPlugins = QPluginLoader::staticInstances();

    for (QObject *obj : std::as_const(staticPlugins)) {
        if (IAddressBookFactory *addrPlugin = qobject_cast<IAddressBookFactory *>(obj)) {
            auto const configs = addrPlugin->configurations();
            qCInfo(lcAddressBookManager)
                    << "Found" << configs.length()
                    << "active configurations for address book plugin" << addrPlugin->name();

            for (auto &cfg : std::as_const(configs)) {
                m_addressBookFeeders.insert(cfg, addrPlugin->createFeeder(cfg, this));
                m_addressBookConfigs.push_back(cfg);
            }
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
    bool changed = false;
    bool networkAvailable = true;
    auto &nh = NetworkHelper::instance();

    if (!m_queueMutex.tryLock()) {
        QTimer::singleShot(100, this, &AddressBookManager::processAddressBookQueue);
        return;
    }

    QMutableStringListIterator it(m_addressBookQueue);
    while (it.hasNext()) {
        QString group = it.next();

        if (auto feeder = m_addressBookFeeders.value(group, nullptr)) {

            // If the plugin requires network access, check the connectivity with
            // the network helper / portal. If we've no connectivity, trigger on
            // connectivityChanged signal to recheck again.
            QUrl checkURL = feeder->networkCheckURL();
            if (!checkURL.isEmpty()) {
                if (!networkAvailable) {
                    continue;
                }

                if (!nh.hasConnectivity()) {
                    qCWarning(lcAddressBookManager) << "no connectivity state yet - trying later";

                    networkAvailable = false;
                    connect(
                            &nh, &NetworkHelper::connectivityChanged, this,
                            [this]() { processAddressBookQueue(); },
                            Qt::ConnectionType::SingleShotConnection);

                    continue;
                }

                if (!nh.isReachable(checkURL)) {
                    qCWarning(lcAddressBookManager) << checkURL << "is not reachable";
                    connect(
                            &nh, &NetworkHelper::connectivityChanged, this,
                            [this]() { processAddressBookQueue(); },
                            Qt::ConnectionType::SingleShotConnection);
                    continue;
                }
            }

            feeder->process();
            it.remove();
            changed = true;
        }
    }

    if (changed) {
        Q_EMIT AddressBook::instance().contactsReady();
    }

    m_queueMutex.unlock();
}

void AddressBookManager::acquireSecret(const QString &group,
                                       std::function<void(const QString &secret)> callback)
{
    ReadOnlyConfdSettings settings;

    const auto groupHash = settings.hashForSettingsGroup(group);
    const auto secretKey = QString("%1_%2").arg(group, groupHash);

    Credentials::instance().get(
            secretKey + "/secret",
            [this, group, secretKey, callback](bool error, const QString &secret) {
                if (error) {
                    qCWarning(lcAddressBookManager) << "failed to retrieve secret:" << secret;
                    return;
                }

                if (secret.isEmpty()) {
                    auto &viewHelper = ViewHelper::instance();

                    auto conn = connect(
                            &viewHelper, &ViewHelper::passwordResponded, this,
                            [secretKey, group, callback, this](const QString &id,
                                                               const QString &password) {
                                if (id == group) {
                                    QObject::disconnect(m_viewHelperConnections.value(group));
                                    m_viewHelperConnections.remove(group);

                                    Credentials::instance().set(
                                            secretKey + "/secret", password,
                                            [secretKey](bool error, const QString &data) {
                                                if (error) {
                                                    qCCritical(lcAddressBookManager)
                                                            << "failed to set credentials:" << data;
                                                }
                                            });

                                    callback(password);
                                }
                            });

                    m_viewHelperConnections.insert(group, conn);

                    ReadOnlyConfdSettings settings;
                    settings.beginGroup(group);
                    viewHelper.requestPassword(group, settings.value("host", "").toString());
                    settings.beginGroup(group);

                } else {
                    callback(secret);
                }
            });
}
