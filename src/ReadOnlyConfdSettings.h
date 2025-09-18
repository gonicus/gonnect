#pragma once
#include <QSettings>

class ReadOnlyConfdSettings : public QSettings
{
    Q_OBJECT

public:
    explicit ReadOnlyConfdSettings(QObject *parent = nullptr);
    ~ReadOnlyConfdSettings() = default;

    QString hashForSettingsGroup(const QString &group);

private:
#ifdef Q_OS_LINUX
    QString gidToName(gid_t gid);
    QStringList getUserGroups();
#endif

    void readConfd();
};
