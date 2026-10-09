#include "ChatUserPresenceStateProvider.h"
#include "ChatUser.h"

ChatUserPresenceStateProvider::ChatUserPresenceStateProvider(ChatUser *chatUser)
    : IPresenceStateProvider{ chatUser }
{
    Q_CHECK_PTR(chatUser);
    m_chatUser = chatUser;

    connect(chatUser, &ChatUser::presenceStateChanged, this,
            &ChatUserPresenceStateProvider::updateState);
    updateState();
}

void ChatUserPresenceStateProvider::updateState()
{
    Q_CHECK_PTR(m_chatUser);

    using State = PresenceState::State;

    if (m_chatUser->hasPresenceState()) {
        switch (m_chatUser->presenceState()) {

        case ChatUser::PresenceState::Unknown:
            setPresenceState(State::Unknown);
            return;

        case ChatUser::PresenceState::Offline:
            setPresenceState(State::Offline);
            return;

        case ChatUser::PresenceState::Away:
            setPresenceState(State::Away);
            return;

        case ChatUser::PresenceState::Online:
            setPresenceState(State::Available);
            return;
        }
    }

    setPresenceState(State::Unknown);
}
