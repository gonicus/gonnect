#include <QDir>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QLoggingCategory>
#include <QCryptographicHash>
#include <unistd.h>
#include <grp.h>
#include "ReadOnlyConfdSettings.h"

Q_LOGGING_CATEGORY(lcReadOnlySettings, "gonnect.app.settings")

ReadOnlyConfdSettings::ReadOnlyConfdSettings(QObject *parent)
    : QSettings("/dev/null", QSettings::IniFormat, parent)
{
    setFallbacksEnabled(false);
    readConfd();
}

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

void ReadOnlyConfdSettings::readConfd()
{
    static const QRegularExpression configFileName("^\\d+-[a-zA-Z0-9_-]+.conf$");
    static const QRegularExpression envPlaceholder("%ENV\\[([a-zA-Z0-9][A-Za-z0-9_]*)\\]%");
    static const QRegularExpression cfgPlaceholder("%CFG\\[([A_Za-z_/]+)\\]%");

    const QString basePath =
            QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/gonnect";

    const auto baseDir = QDir(basePath);
    const QStringList entries = baseDir.entryList(QDir::Files | QDir::Readable, QDir::Name);

    // Filter scope and replace %ENV[variablename]% and %CONF[config/key]% placeholders
    const auto groupList = getUserGroups();

    for (auto &entry : std::as_const(entries)) {
        if (configFileName.match(entry).hasMatch()) {

            const QSettings tmpSettings(basePath + "/" + entry, QSettings::IniFormat);

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

                QString settingsValue = tmpSettings.value(key).toString();
                auto envMatch = envPlaceholder.match(settingsValue);
                if (envMatch.hasMatch()) {
                    settingsValue.replace(
                            envPlaceholder,
                            qEnvironmentVariable(envMatch.captured(1).toStdString().c_str()));
                }
                auto cfgMatch = cfgPlaceholder.match(settingsValue);
                if (cfgMatch.hasMatch()) {
                    settingsValue.replace(cfgPlaceholder, value(cfgMatch.captured(1)).toString());
                }

                setValue(key, settingsValue);
            }
        }
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
