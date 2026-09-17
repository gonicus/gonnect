#include "AvatarPrioHelper.h"
#include "ChatConnectorManager.h"
#include "AddressBookManager.h"
#include "ReadOnlyConfdSettings.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcAvatarPrioHelper, "gonnect.helper.AvatarPrioHelper")

static constexpr uint DEFAULT_PRIO = 50;

uint AvatarPrioHelper::prioFor(const QString &configId) const
{
    QReadLocker locker(&m_priosLock);

    if (!m_prios.contains(configId)) {
        QMutexLocker warnedLocker(&m_warnedConfigIdsLock);
        if (!m_warnedConfigIds.contains(configId)) {
            m_warnedConfigIds.insert(configId);
            qCWarning(lcAvatarPrioHelper)
                    << "Unknown config id" << configId << "returning default value" << DEFAULT_PRIO;
        }
        return DEFAULT_PRIO;
    }

    return m_prios.value(configId, DEFAULT_PRIO);
}

AvatarPrioHelper::AvatarPrioHelper(QObject *parent) : QObject{ parent }
{
    connect(&ChatConnectorManager::instance(), &ChatConnectorManager::chatConnectorsChanged, this,
            &AvatarPrioHelper::updatePriosFromConfig);

    connect(&AddressBookManager::instance(), &AddressBookManager::configsLoaded, this,
            &AvatarPrioHelper::updatePriosFromConfig);

    updatePriosFromConfig();
}

void AvatarPrioHelper::updatePriosFromConfig()
{
    QHash<QString, uint> prios;

    ReadOnlyConfdSettings settings;

    const auto addrConfigs = AddressBookManager::instance().addressBookConfigs();
    for (const auto &config : addrConfigs) {
        const auto prio =
                settings.value(QString("%1/avatarPrio").arg(config), DEFAULT_PRIO).toUInt();
        prios.insert(config, prio);

        qCInfo(lcAvatarPrioHelper) << "Avatar priority for" << config << "is" << prio;
    }

    const auto chatConnectors = ChatConnectorManager::instance().chatConnectors();
    for (const auto *connector : chatConnectors) {
        const auto id = connector->id();
        const auto prio = settings.value(QString("%1/avatarPrio").arg(id), DEFAULT_PRIO).toUInt();
        prios.insert(id, prio);

        qCInfo(lcAvatarPrioHelper) << "Avatar priority for" << id << "is" << prio;
    }

    bool changed = false;
    {
        QWriteLocker locker(&m_priosLock);
        if (m_prios != prios) {
            m_prios = prios;
            changed = true;
        }
    }

    if (changed) {
        Q_EMIT priosChanged();
    }
}
