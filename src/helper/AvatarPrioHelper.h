#pragma once

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QReadWriteLock>
#include <QSet>

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

Q_SIGNALS:
    void priosChanged();

private:
    explicit AvatarPrioHelper(QObject *parent = nullptr);

    QHash<QString, uint> m_prios;
    mutable QReadWriteLock m_priosLock;
    mutable QSet<QString> m_warnedConfigIds;
    mutable QMutex m_warnedConfigIdsLock;

private Q_SLOTS:
    void updatePriosFromConfig();
};
