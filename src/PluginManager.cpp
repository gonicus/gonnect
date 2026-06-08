#include "PluginManager.h"

#ifdef Q_OS_FLATPAK
#  include <QDir>
#  include <QDirListing>
#  include <QSettings>
#else
#  include "ReadOnlyConfdSettings.h"
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPluginManager, "gonnect.app.plugins.Manager")

PluginManager::PluginManager(QObject *parent) : QObject{ parent } { }

PluginManager::~PluginManager()
{
    qDeleteAll(m_pluginInfos);
}

#define GONNECT_ASSERT_KEY(key)                                                         \
    if (!keys.contains(key)) {                                                          \
        qCWarning(lcPluginManager)                                                      \
                << "Settings group" << group << "does not contain mandatory key" << key \
                << "and will be ignored in file" << absPath;                            \
        settings.endGroup();                                                            \
        continue;                                                                       \
    }

void PluginManager::init()
{
#ifdef Q_OS_FLATPAK

    using F = QDirListing::IteratorFlag;
    qCInfo(lcPluginManager) << "Performing plugin search...";

    for (const auto &dirEntry :
         QDirListing("/app/plugins", { "plugin.info" }, F::FilesOnly | F::Recursive)) {

        const QString &absPath = dirEntry.absoluteFilePath();

        QSettings settings(absPath, QSettings::IniFormat);
        const auto groups = settings.childGroups();
        if (groups.isEmpty()) {
            qCWarning(lcPluginManager) << "Found no plugin config groups in" << absPath;
            continue;
        }

        for (const auto &group : groups) {
            settings.beginGroup(group);

            const auto keys = settings.childKeys();
            GONNECT_ASSERT_KEY("type")
            GONNECT_ASSERT_KEY("executable")
            GONNECT_ASSERT_KEY("version")

            auto pluginInfo = new PluginInfo;
            pluginInfo->binPath =
                    QDir::cleanPath(QString("%1/bin/%2")
                                            .arg(dirEntry.absolutePath(),
                                                 settings.value("executable", "").toString()));
            pluginInfo->version = settings.value("version", "").toString();
            pluginInfo->type = settings.value("type", "").toString();
            pluginInfo->displayName = settings.value("displayName", group).toString();
            m_pluginInfos.append(pluginInfo);

            qCInfo(lcPluginManager).noquote().nospace()
                    << "Found plugin '" << pluginInfo->displayName << "', version "
                    << pluginInfo->version << ", type '" << pluginInfo->type << "' and binary path "
                    << pluginInfo->binPath;

            settings.endGroup();
        }
    }

    qCInfo(lcPluginManager) << "PluginManager initialized: found" << m_pluginInfos.size()
                            << "plugins";

#else
    ReadOnlyConfdSettings settings;
    auto pluginInfo = new PluginInfo;
    pluginInfo->binPath = settings.value("generic/headlessClientPath", "").toString();
    pluginInfo->displayName = "YAMA";
    pluginInfo->version = "0.1.0";
    pluginInfo->type = "chat-ipc";
    m_pluginInfos.append(pluginInfo);
#endif

    m_isInitialized = true;
}

#undef GONNECT_ASSERT_KEY

const PluginInfo *PluginManager::pluginByType(const QString &type)
{
    if (!m_isInitialized) {
        init();
    }

    for (const auto &pluginInfo : std::as_const(m_pluginInfos)) {
        if (pluginInfo->type == type) {
            return pluginInfo;
        }
    }

    return nullptr;
}
