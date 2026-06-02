#pragma once

#include <QObject>
#include "PresenceState.h"
#include "IPresenceStateProvider.h"

/*!
 * \brief The PresenceStateAggregator class aggregates the presence state of an entity (e.g. user)
 * out of different sub providers.
 *
 * \warning Objects of this class must not be created or used directly! Instead, they must only be
 * acquired via the PresenceStateObjectManager singleton and used via PresenceStateProxy!
 *
 * An entity (like a user) can appear in different sub systems (e.g. as a SIP and as a chat client).
 * It may have different presence states in those sub systems.
 *
 * This class aggregates the different presence states into a global one.
 *
 * The individual sub states are represented by the IPresenceStateProvider objects that must be
 * registered with this object.
 */
class PresenceStateAggregator : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    explicit PresenceStateAggregator(QObject *parent = nullptr);

    void registerStateProvider(IPresenceStateProvider *provider);

    PresenceState::State presenceState() const { return m_presenceState; }
    QString stateText() const { return m_stateText; }

private Q_SLOTS:
    void updateAggregatedState();

private:
    QSet<IPresenceStateProvider *> m_stateProviders;
    PresenceState::State m_presenceState;
    QString m_stateText;

Q_SIGNALS:
    void presenceStateChanged();
    void stateTextChanged();
};
