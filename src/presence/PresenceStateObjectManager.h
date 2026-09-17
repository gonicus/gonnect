#pragma once

#include <QObject>
#include "Contact.h"

class PresenceStateAggregator;

class PresenceStateObjectManager : public QObject
{
    Q_OBJECT

public:
    static PresenceStateObjectManager &instance()
    {
        static PresenceStateObjectManager *_instance = nullptr;
        if (!_instance) {
            _instance = new PresenceStateObjectManager;
        }
        return *_instance;
    };
    ~PresenceStateObjectManager();

    PresenceStateAggregator *acquire(Contact *contact);
    void release(Contact *contact);

private:
    explicit PresenceStateObjectManager(QObject *parent = nullptr);

    struct RefCount
    {
        PresenceStateAggregator *stateObj = nullptr;
        quint32 count = 0;
    };

    QHash<Contact *, RefCount *> m_refs;
};
