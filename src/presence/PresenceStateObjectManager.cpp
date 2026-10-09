#include "PresenceStateObjectManager.h"

PresenceStateObjectManager::PresenceStateObjectManager(QObject *parent) : QObject{ parent } { }

PresenceStateObjectManager::~PresenceStateObjectManager()
{
    qDeleteAll(m_refs);
}

PresenceStateAggregator *PresenceStateObjectManager::acquire(Contact *contact)
{
    if (!contact) {
        return nullptr;
    }

    if (auto *refCount = m_refs.value(contact, nullptr)) {
        refCount->count++;
        return refCount->stateObj;
    } else {
        auto *refCountObj = new RefCount;
        refCountObj->stateObj = contact->createPresenceStateObject();
        refCountObj->count = 1;
        refCountObj->stateObj->setParent(this);
        m_refs.insert(contact, refCountObj);

        connect(contact, &QObject::destroyed, this, [this, contact](QObject *) {
            if (auto refCount = m_refs.take(contact)) {
                refCount->stateObj->deleteLater();
                delete refCount;
            }
        });

        return refCountObj->stateObj;
    }
}

void PresenceStateObjectManager::release(Contact *contact)
{
    if (!contact) {
        return;
    }

    if (auto *refCount = m_refs.value(contact, nullptr)) {
        if (refCount->count == 1) {
            contact->disconnect(this);
            m_refs.remove(contact);
            refCount->stateObj->deleteLater();
            delete refCount;
        } else {
            refCount->count--;
        }
    }
}
