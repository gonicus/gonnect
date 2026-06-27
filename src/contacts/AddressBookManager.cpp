#include "AddressBookManager.h"
#include "ReadOnlyConfdSettings.h"
#include "AddressBook.h"
#include "KeychainSettings.h"
#include "IAddressBookFactory.h"
#include "ViewHelper.h"
#include "NetworkHelper.h"
#include "Credentials.h"
#include "ErrorBus.h"

#include <QTimer>
#include <QUrl>
#include <QRegularExpression>
#include <QLoggingCategory>
#include <QCryptographicHash>
#include <QPluginLoader>
#include <QMutexLocker>

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
    int retryCount = settings.value("generic/feederPluginRetryCount", 5).toInt();
    int retryInterval = settings.value("generic/feederPluginRetryInterval", 10000).toInt();

    const QObjectList &staticPlugins = QPluginLoader::staticInstances();

    for (QObject *obj : std::as_const(staticPlugins)) {
        if (IAddressBookFactory *addrPlugin = qobject_cast<IAddressBookFactory *>(obj)) {
            auto const configs = addrPlugin->configurations();
            qCInfo(lcAddressBookManager)
                    << "Found" << configs.length()
                    << "active configurations for address book plugin" << addrPlugin->name();

            for (auto &cfg : std::as_const(configs)) {
                m_addressBookFeeders.insert(
                        cfg, addrPlugin->createFeeder(cfg, retryCount, retryInterval, this));
                m_addressBookConfigs.push_back(cfg);
            }
        }
    }
}

void AddressBookManager::reloadAddressBook()
{
    AddressBook::instance().resetContacts();
    m_addressBookQueue = m_addressBookConfigs;
    processAddressBookQueue();
}

void AddressBookManager::processAddressBookQueue()
{
    bool networkAvailable = true;
    auto &nh = NetworkHelper::instance();

    if (!m_queueMutex.tryLock()) {
        qCFatal(lcAddressBookManager) << "Failed to acquire lock for the feeder queue";
        return;
    }

    QMutableStringListIterator it(m_addressBookQueue);
    while (it.hasNext()) {
        QString group = it.next();

        if (auto feeder = m_addressBookFeeders.value(group, nullptr)) {
            if (feeder->isProcessing()) {
                // A currently active feeder must not be invoked again to prevent double runs and
                // threading issues.
                continue;
            }

            // If the plugin requires network access, check the connectivity with
            // the network helper / portal. If we've no connectivity, trigger on
            // connectivityChanged signal to recheck again.
            QUrl checkURL = feeder->networkCheckURL();

            if (checkURL.isEmpty()) {
                feeder->process();
            } else {
                if (!networkAvailable) {
                    continue;
                }

                if (!checkURL.isValid()) {
                    qCCritical(lcAddressBookManager) << "URL is invalid:" << checkURL;

                    continue;
                }

                if (!nh.hasConnectivity()) {
                    qCWarning(lcAddressBookManager) << "No connectivity state yet - trying later";

                    networkAvailable = false;
                    scheduleReconnect();
                    continue;
                }

                nh.isReachable(checkURL).then(
                        this, [this, feeder, group, checkURL](bool isReachable) {
                            if (isReachable) {
                                feeder->process();
                                Q_EMIT AddressBook::instance().contactsReady();
                            } else {
                                qCWarning(lcAddressBookManager)
                                        << "Feeder URL" << checkURL << "is not reachable";

                                requeueGroup(group);
                                scheduleReconnect();
                            }
                        });
            }

            it.remove();
        }
    }

    m_queueMutex.unlock();
}

void AddressBookManager::requeueGroup(const QString &group)
{
    QMutexLocker locker(&m_queueMutex);
    if (!m_addressBookQueue.contains(group)) {
        m_addressBookQueue.append(group);
    }
}

void AddressBookManager::scheduleReconnect()
{
    if (m_reconnectScheduled) {
        return;
    }

    m_reconnectScheduled = true;
    connect(
            &NetworkHelper::instance(), &NetworkHelper::connectivityChanged, this,
            [this]() {
                m_reconnectScheduled = false;
                processAddressBookQueue();
            },
            Qt::ConnectionType::SingleShotConnection);
}

void AddressBookManager::acquireSecret(bool forcePrompt, const QString &group,
                                       std::function<void(const QString &secret)> callback)
{
    ReadOnlyConfdSettings settings;

    const auto groupHash = settings.hashForSettingsGroup(group);
    const auto secretKey = QString("%1_%2").arg(group, groupHash);

    Credentials::instance().get(
            secretKey + "/secret",
            [this, forcePrompt, group, secretKey,
             callback](QKeychain::Error error, const QString &secret, const QString &) {
                if (error == QKeychain::NoError && !forcePrompt) {
                    callback(secret);
                } else if (error == QKeychain::EntryNotFound || forcePrompt) {
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
                                            [secretKey](QKeychain::Error error, const QString &,
                                                        const QString &message) {
                                                if (error != QKeychain::NoError) {
                                                    ErrorBus::instance().error(
                                                            tr("Failed to persist address book "
                                                               "credentials: %1")
                                                                    .arg(message));
                                                }
                                            });

                                    callback(password);
                                }
                            });

                    m_viewHelperConnections.insert(group, conn);

                    ReadOnlyConfdSettings settings;
                    settings.beginGroup(group);
                    viewHelper.requestPassword(group, settings.value("host", "").toString());
                    settings.endGroup();
                }
            });
}
