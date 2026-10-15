#include "PresenceStateProxy.h"
#include "AddressBook.h"
#include "PresenceStateObjectManager.h"

PresenceStateProxy::PresenceStateProxy(QObject *parent) : QObject{ parent }
{
    connect(this, &PresenceStateProxy::contactIdChanged, this,
            &PresenceStateProxy::updateStateObject);
}

PresenceStateProxy::~PresenceStateProxy()
{
    if (m_aggregator && m_contact) {
        PresenceStateObjectManager::instance().release(m_contact);
    }
}

PresenceState::State PresenceStateProxy::presenceState() const
{
    return m_aggregator ? m_aggregator->presenceState() : PresenceState::State::Unknown;
}

QString PresenceStateProxy::stateText() const
{
    return m_aggregator ? m_aggregator->stateText() : "";
}

void PresenceStateProxy::updateStateObject()
{
    if (m_context) {
        m_context->deleteLater();
        m_context = nullptr;
    }

    if (m_aggregator) {
        PresenceStateObjectManager::instance().release(m_contact);
        m_aggregator = nullptr;
    }

    if (m_contactId.isEmpty()) {
        m_contact = nullptr;
    } else {
        m_contact = AddressBook::instance().lookupByContactId(m_contactId);
        if (m_contact) {
            m_context = new QObject(this);
            m_aggregator = PresenceStateObjectManager::instance().acquire(m_contact);

            connect(m_aggregator, &QObject::destroyed, m_context,
                    [this](QObject *) { setProperty("contact", QVariant::fromValue(nullptr)); });

            connect(m_aggregator, &PresenceStateAggregator::presenceStateChanged, m_context,
                    [this]() { Q_EMIT presenceStateChanged(); });
            connect(m_aggregator, &PresenceStateAggregator::stateTextChanged, m_context,
                    [this]() { Q_EMIT stateTextChanged(); });
        }
    }

    Q_EMIT presenceStateChanged();
    Q_EMIT stateTextChanged();
}
