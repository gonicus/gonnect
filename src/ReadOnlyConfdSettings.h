#pragma once
#include <QSettings>

class ReadOnlyConfdSettings : public QSettings
{
    Q_OBJECT

public:
    explicit ReadOnlyConfdSettings(QObject *parent = nullptr);
    ~ReadOnlyConfdSettings() = default;

    QString hashForSettingsGroup(const QString &group);

    static void invalidateCache();

private:
    void readConfd();
};
