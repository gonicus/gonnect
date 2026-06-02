#pragma once

#include "IPresenceStateProvider.h"

class ChatUser;

class ChatUserPresenceStateProvider : public IPresenceStateProvider
{
    Q_OBJECT

public:
    explicit ChatUserPresenceStateProvider(ChatUser *chatUser);

private Q_SLOTS:
    void updateState();

private:
    ChatUser *m_chatUser = nullptr;
};
