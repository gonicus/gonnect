#pragma once
#include <QSettings>
#include <QStandardPaths>
#include <QLibraryInfo>

class AppSettings : public QSettings
{
    Q_OBJECT

public:
    explicit AppSettings(QObject *parent = nullptr)
        : QSettings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                            + "/gonnect/99-user.conf",
                    QSettings::IniFormat, parent)
    {
    }
    ~AppSettings() = default;
};
