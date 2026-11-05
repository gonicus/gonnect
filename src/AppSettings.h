#pragma once
#include <QSettings>
#include <QStandardPaths>
#include <QLibraryInfo>

#include "ReadOnlyConfdSettings.h"

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

    void setValue(QAnyStringView key, const QVariant &value)
    {
        QSettings::setValue(key, value);
        invalidateConfdCache();
    }

    void remove(QAnyStringView key)
    {
        QSettings::remove(key);
        invalidateConfdCache();
    }

    void clear()
    {
        QSettings::clear();
        invalidateConfdCache();
    }

private:
    void invalidateConfdCache()
    {
        sync();
        ReadOnlyConfdSettings::invalidateCache();
    }
};
