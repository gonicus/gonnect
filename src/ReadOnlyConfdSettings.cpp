#include <QDir>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QLoggingCategory>
#include <QCryptographicHash>
#include <QMutex>

#ifdef Q_OS_WINDOWS
static constexpr const char *NULL_DEVICE_NAME = "nul";
#else
#  include <unistd.h>
#  include <grp.h>
static constexpr const char *NULL_DEVICE_NAME = "/dev/null";
#endif

#include "ReadOnlyConfdSettings.h"

Q_LOGGING_CATEGORY(lcReadOnlySettings, "gonnect.app.settings")

namespace {
#ifdef Q_OS_LINUX
QString gidToName(gid_t gid)
{
    struct group *g;
    g = getgrgid(gid);

    if (g == NULL) {
        return "";
    }

    return g->gr_name;
}

QStringList getUserGroups()
{
    QStringList res;

    auto groupCount = getgroups(0, nullptr);

    std::vector<gid_t> gidList(groupCount);
    if (groupCount > 0) {
        groupCount = getgroups(gidList.size(), &gidList[0]);
    }

    for (auto i = 0; i < groupCount; ++i) {
        const QString groupName = gidToName(gidList[i]);
        if (!groupName.isEmpty()) {
            res.push_back(groupName);
        }
    }

    return res;
};
#endif

QString replacePlaceholders(const QString &settingsStringValue, const QVariantMap &resolved)
{
    static const QRegularExpression envPlaceholder(R"(%ENV\[([a-zA-Z][A-Za-z0-9_]*)\]%)");
    static const QRegularExpression cfgPlaceholder(R"(%CFG\[([A-Za-z0-9_/.\-]+)\]%)");

    QString str = settingsStringValue;

    // Iterate backwards because replacements would change match positions
    auto envIt = envPlaceholder.globalMatch(str);
    QList<QRegularExpressionMatch> envList;
    while (envIt.hasNext()) {
        envList.append(envIt.next());
    }
    for (qsizetype i = envList.size() - 1; i >= 0; --i) {
        const auto envMatch = envList.at(i);
        const auto value = qEnvironmentVariable(envMatch.captured(1).toStdString().c_str());
        str.replace(envMatch.capturedStart(0), envMatch.capturedLength(0), value);
    }

    auto cfgIt = cfgPlaceholder.globalMatch(str);
    QList<QRegularExpressionMatch> cfgList;
    while (cfgIt.hasNext()) {
        cfgList.append(cfgIt.next());
    }
    for (qsizetype i = cfgList.size() - 1; i >= 0; --i) {
        const auto cfgMatch = cfgList.at(i);
        str.replace(cfgMatch.capturedStart(0), cfgMatch.capturedLength(0),
                    resolved.value(cfgMatch.captured(1)).toString());
    }

    return str;
}

QStringList scanConfd()
{
    static const QRegularExpression configFileName(R"(\d+-[a-zA-Z0-9_-]+\.conf$)");

    QStringList dirs;
    if (qEnvironmentVariable("container") == "flatpak") {
        dirs += QStringLiteral("/app/etc/gonnect");
    }

    dirs += QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/gonnect";

    QStringList entries;

    for (const auto &dirPath : std::as_const(dirs)) {
        const QDir dir(dirPath);
        const auto files = dir.entryList(QDir::Files | QDir::Readable, QDir::Name);

        for (const auto &file : files) {
            const auto path = dir.absoluteFilePath(file);
            if (configFileName.match(path).hasMatch()) {
                entries += path;
            }
        }
    }

    return entries;
}

QVariantMap buildEntries(const QStringList &files)
{
#ifdef Q_OS_LINUX
    const auto groupList = getUserGroups();
#else
    const auto groupList = getUserGroups();
#endif

    QVariantMap resolved;
    for (const auto &file : files) {
        const QSettings tmpSettings(file, QSettings::IniFormat);

        // Check if the configuration snippet is relevant to us
        const QString onlyForGroup = tmpSettings.value("scope/group").toString();
        if (!onlyForGroup.isEmpty() && !groupList.contains(onlyForGroup)) {
            continue;
        }

        // Copy over all keys, overwriting older values if desired
        const QStringList keys = tmpSettings.allKeys();
        for (const auto &key : keys) {
            if (key.startsWith("scope/")) {
                continue;
            }

            const QVariant settingsValue = tmpSettings.value(key);

            if (settingsValue.userType() == QMetaType::QStringList) {
                QStringList newList;
                const QStringList strings = settingsValue.toStringList();
                newList.reserve(strings.length());
                std::ranges::transform(
                        strings, std::back_inserter(newList),
                        [&resolved](const QString &s) { return replacePlaceholders(s, resolved); });

                resolved.insert(key, newList);

            } else if (settingsValue.userType() == QMetaType::QString) {
                resolved.insert(key, replacePlaceholders(settingsValue.toString(), resolved));
            } else {
                resolved.insert(key, settingsValue);
            }
        }
    }

    return resolved;
}

struct ConfdCache
{
    QMutex mutex;
    bool valid = false;
    QVariantMap entries;
};

Q_GLOBAL_STATIC(ConfdCache, confdCache)

QVariantMap mergedEntries()
{
    auto *cache = confdCache();
    if (!cache) {
        return {};
    }

    const QMutexLocker locker(&cache->mutex);

    if (!cache->valid) {
        const auto files = scanConfd();
        cache->entries = buildEntries(files);
        cache->valid = true;
    }

    return cache->entries;
}

} // namespace

ReadOnlyConfdSettings::ReadOnlyConfdSettings(QObject *parent)
    : QSettings(NULL_DEVICE_NAME, QSettings::IniFormat, parent)
{
    setFallbacksEnabled(false);
    readConfd();
}

void ReadOnlyConfdSettings::readConfd()
{
    const QVariantMap entries = mergedEntries();

    for (auto it = entries.cbegin(), end = entries.cend(); it != end; ++it) {
        setValue(it.key(), it.value());
    }
}

void ReadOnlyConfdSettings::invalidateCache()
{
    if (auto *cache = confdCache()) {
        const QMutexLocker locker(&cache->mutex);
        cache->valid = false;
        cache->entries.clear();
    }
}

QString ReadOnlyConfdSettings::hashForSettingsGroup(const QString &group)
{
    beginGroup(group);

    QString groupSettingsStr;
    auto ck = childKeys();
    std::sort(ck.begin(), ck.end());

    for (const auto &key : std::as_const(ck)) {
        groupSettingsStr.append(key);
        groupSettingsStr.append(value(key, "").toString());
    }

    endGroup();

    return QCryptographicHash::hash(groupSettingsStr.toUtf8(), QCryptographicHash::Md5).toHex();
}
