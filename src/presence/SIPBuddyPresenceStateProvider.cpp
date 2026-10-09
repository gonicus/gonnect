#include "SIPBuddyPresenceStateProvider.h"
#include "SIPBuddy.h"
#include "SIPManager.h"

SIPBuddyPresenceStateProvider::SIPBuddyPresenceStateProvider(const QString &sipUrl, QObject *parent)
    : IPresenceStateProvider{ parent }, m_sipUrl{ sipUrl }
{
    Q_ASSERT(!sipUrl.isEmpty());

    connect(&SIPManager::instance(), &SIPManager::buddyStateChanged, this,
            [this](const QString url, SIPBuddyState::STATUS) {
                if (m_sipUrl.compare(url, Qt::CaseInsensitive) == 0) {
                    updateState();
                }
            });
    updateState();
}

void SIPBuddyPresenceStateProvider::updateState()
{
    const auto state = SIPManager::instance().buddyStatus(m_sipUrl);
    using State = PresenceState::State;

    switch (state) {

    case SIPBuddyState::UNKNOWN:
        setPresenceState(State::Unknown);
        break;
    case SIPBuddyState::UNAVAILABLE:
        setPresenceState(State::Offline);
        break;
    case SIPBuddyState::READY:
        setPresenceState(State::Available);
        break;
    case SIPBuddyState::RINGING:
        setPresenceState(State::Ringing);
        break;
    case SIPBuddyState::BUSY:
        setPresenceState(State::Busy);
        break;
    }

    // setStateText(m_sipBuddy->statusText());
}
