#pragma once
#include <QSettings>
#include <QStandardPaths>
#include <QLibraryInfo>

class KeychainSettings : public QSettings
{
    Q_OBJECT

public:
    explicit KeychainSettings(QObject *parent = nullptr)
        : QSettings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                            + "/gonnect/keychain",
                    QSettings::IniFormat, parent)
    {
    }

    static QString secret(const QString &group);

    ~KeychainSettings() = default;
};
