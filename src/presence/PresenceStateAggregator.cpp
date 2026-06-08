#include "PresenceStateAggregator.h"
#include "IPresenceStateProvider.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPresenceState, "gonnect.app.contact.PresenceState")

PresenceStateAggregator::PresenceStateAggregator(QObject *parent) : QObject{ parent } { }

void PresenceStateAggregator::registerStateProvider(IPresenceStateProvider *provider)
{
    if (!provider) {
        qCWarning(lcPresenceState) << "Cannot register nullptr - ignoring";
        return;
    }
    if (m_stateProviders.contains(provider)) {
        qCWarning(lcPresenceState) << "Provider already registered - ignoring";
        return;
    }
    m_stateProviders.insert(provider);

    // Listeners
    connect(provider, &IPresenceStateProvider::presenceStateChanged, this,
            &PresenceStateAggregator::updateAggregatedState);
    connect(provider, &IPresenceStateProvider::stateTextChanged, this,
            &PresenceStateAggregator::updateAggregatedState);
    connect(provider, &QObject::destroyed, this, [this](QObject *obj) {
        m_stateProviders.remove(static_cast<IPresenceStateProvider *>(obj));
        updateAggregatedState();
    });

    updateAggregatedState();
}

void PresenceStateAggregator::updateAggregatedState()
{
    using State = PresenceState::State;

    static const QMap<State, quint8> prios = {
        { State::Unknown, 0 },
        { State::Available, 1 },
        { State::Away, 2 },
        { State::Busy, 3 },
    };

    State newState = State::Unknown;
    quint8 currPrio = 0;
    QStringList newTexts;

    // Collect and aggregate states
    for (const auto *provider : std::as_const(m_stateProviders)) {
        const auto currState = provider->presenceState();
        const auto newPrio = prios.value(currState);
        if (newPrio > currPrio) {
            currPrio = newPrio;
            newState = currState;
        }

        if (!provider->stateText().isEmpty()) {
            newTexts.append(provider->stateText());
        }
    }

    // Update properties
    if (m_presenceState != newState) {
        m_presenceState = newState;
        Q_EMIT presenceStateChanged();
    }

    const auto newText = newTexts.join("; ");
    if (m_stateText != newText) {
        m_stateText = newText;
        Q_EMIT stateTextChanged();
    }
}
