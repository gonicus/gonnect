#include "GlobalInfo.h"

#include "ReadOnlyConfdSettings.h"

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
    if (m_workaroundActiveCache.contains(id)) {
        return m_workaroundActiveCache.value(id);
    }

    ReadOnlyConfdSettings settings;
    const auto idStr = GlobalInfo::workaroundIdToString(id);
    const bool value = settings.value("workarounds/" + idStr, false).toBool();
    m_workaroundActiveCache.insert(id, value);
    return value;
}
