#pragma once

#include <QObject>
#include <QHash>
#include <QReadWriteLock>

class AvatarPrioHelper : public QObject
{
    Q_OBJECT

public:
    static AvatarPrioHelper &instance()
    {
        static AvatarPrioHelper _instance;
        return _instance;
    }

    uint prioFor(const QString &configId) const;

private:
    explicit AvatarPrioHelper(QObject *parent = nullptr);

    QHash<QString, uint> m_prios;
    mutable QReadWriteLock m_priosLock;

private Q_SLOTS:
    void updatePriosFromConfig();
};
