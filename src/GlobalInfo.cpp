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
