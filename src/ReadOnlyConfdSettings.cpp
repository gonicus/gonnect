#include <QDir>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QLoggingCategory>
#include <QCryptographicHash>

#ifdef Q_OS_WINDOWS
static constexpr const char *NULL_DEVICE_NAME = "nul";
#else
#  include <unistd.h>
#  include <grp.h>
static constexpr const char *NULL_DEVICE_NAME = "/dev/null";
#endif

#include "ReadOnlyConfdSettings.h"

Q_LOGGING_CATEGORY(lcReadOnlySettings, "gonnect.app.settings")

ReadOnlyConfdSettings::ReadOnlyConfdSettings(QObject *parent)
    : QSettings(NULL_DEVICE_NAME, QSettings::IniFormat, parent)
{
    setFallbacksEnabled(false);
    readConfd();
}

#ifdef Q_OS_LINUX
QString ReadOnlyConfdSettings::gidToName(gid_t gid)
{
    struct group *g;
    g = getgrgid(gid);

    if (g == NULL) {
        return "";
    }

    return g->gr_name;
}

QStringList ReadOnlyConfdSettings::getUserGroups()
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

void ReadOnlyConfdSettings::readConfd()
{
    static const QRegularExpression configFileName(R"(\d+-[a-zA-Z0-9_-]+\.conf$)");

    // Collect ini files
    QStringList entries;

    if (qEnvironmentVariable("container") == "flatpak") {
        const auto fpBaseDir = QDir("/app/etc/gonnect");
        const auto files = fpBaseDir.entryList(QDir::Files | QDir::Readable, QDir::Name);
        for (const auto &entry : files) {
            entries += fpBaseDir.absoluteFilePath(entry);
        }
    }

    const auto baseDir =
            QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/gonnect/");
    const auto files = baseDir.entryList(QDir::Files | QDir::Readable, QDir::Name);
    for (const auto &entry : files) {
        entries += baseDir.absoluteFilePath(entry);
    }

#ifdef Q_OS_LINUX
    // Filter scope and replace %ENV[variablename]% and %CONF[config/key]% placeholders
    const auto groupList = getUserGroups();
#else
    const auto groupList = QStringList();
#endif

    for (auto &entry : std::as_const(entries)) {
        if (configFileName.match(entry).hasMatch()) {

            const QSettings tmpSettings(entry, QSettings::IniFormat);

            // Check if the configuration snippet is relevant to us
            const QString onlyForGroup = tmpSettings.value("scope/group").toString();
            if (!onlyForGroup.isEmpty() && !groupList.contains(onlyForGroup)) {
                continue;
            }

            // Copy over all keys, overwriting older values if desired
            const QStringList keys = tmpSettings.allKeys();
            for (auto &key : std::as_const(keys)) {
                if (key.startsWith("scope/")) {
                    continue;
                }

                QVariant settingsValue = tmpSettings.value(key);

                if (settingsValue.userType() == QMetaType::QStringList) {
                    QStringList newList;
                    const QStringList strings = settingsValue.toStringList();
                    newList.reserve(strings.length());
                    std::ranges::transform(
                            strings, std::back_inserter(newList),
                            [this](const QString &s) { return replacePlaceholders(s); });

                    setValue(key, newList);

                } else if (settingsValue.userType() == QMetaType::QString) {
                    setValue(key, replacePlaceholders(settingsValue.toString()));
                } else {
                    setValue(key, settingsValue);
                }
            }
        }
    }
}

QString ReadOnlyConfdSettings::replacePlaceholders(const QString &settingsStringValue) const
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
                    value(cfgMatch.captured(1)).toString());
    }

    return str;
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
