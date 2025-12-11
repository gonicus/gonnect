#include "GlobalInfo.h"
#include "ReadOnlyConfdSettings.h"
#include "EmergencyContact.h"

#include <QRegularExpression>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcGlobalInfo, "gonnect.globalinfo")

GlobalInfo::GlobalInfo(QObject *parent) : QObject{ parent } { }

QString GlobalInfo::jitsiUrl()
{
    if (!m_isJitsiUrlInitialized) {
        m_isJitsiUrlInitialized = true;

        ReadOnlyConfdSettings settings;
        settings.beginGroup("jitsi");
        m_jitsiUrl = settings.value("url", "").toString();
    }
    return m_jitsiUrl;
}

bool GlobalInfo::isWorkaroundActive(const WorkaroundId id)
{
    bool isCMakePreset = false;

    if (m_workaroundActiveCache.contains(id)) {
        return m_workaroundActiveCache.value(id);
    }

    const auto idStr = GlobalInfo::workaroundIdToString(id);

#if defined(ENABLED_WORKAROUNDS)
    QString workarounds(ENABLED_WORKAROUNDS);
    QStringList wids = workarounds.split(":");

    for (const auto &wid : std::as_const(wids)) {
        if (wid == idStr) {
            isCMakePreset = true;
            break;
        }
    }
#endif

    ReadOnlyConfdSettings settings;
    const bool value = settings.value("workarounds/" + idStr, isCMakePreset).toBool();
    if (value) {
        qCWarning(lcGlobalInfo) << "Workaround" << idStr << "is active";
    }
    m_workaroundActiveCache.insert(id, value);
    return value;
}

void GlobalInfo::initEmergencyContacts()
{
    if (m_hasEmergencyNumbersInitialized) {
        return;
    }

    m_hasEmergencyNumbersInitialized = true;

    static QRegularExpression groupRegex = QRegularExpression("^emergency_(?<groupIndex>[0-9]+)$");

    ReadOnlyConfdSettings settings;
    const QStringList groups = settings.childGroups();

    for (const auto &group : groups) {
        const auto matchResult = groupRegex.match(group);
        if (matchResult.hasMatch()) {
            settings.beginGroup(group);
            bool ok;
            const auto index = matchResult.captured("groupIndex").toUInt(&ok);
            if (!ok) {
                qCCritical(lcGlobalInfo) << "Cannot parse index of settings group" << group;
                continue;
            }
            const auto number = settings.value("number").toString();
            const auto displayName = settings.value("displayName").toString();
            settings.endGroup();

            m_emergencyContacts.append(new EmergencyContact(index, number, displayName, this));
        }
    }

    std::sort(m_emergencyContacts.begin(), m_emergencyContacts.end(),
              [](const EmergencyContact *left, const EmergencyContact *right) -> bool {
                  return left->index() < right->index();
              });
}

bool GlobalInfo::hasEmergencyNumbers()
{
    initEmergencyContacts();
    return !m_emergencyContacts.isEmpty();
}

const QList<EmergencyContact *> &GlobalInfo::emergencyContacts()
{
    initEmergencyContacts();
    return m_emergencyContacts;
}
