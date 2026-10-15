#include "ContactHelper.h"
#include "AddressBook.h"

ContactHelper::ContactHelper(QObject *parent) : QObject{ parent } { }

Contact *ContactHelper::lookupByChatUser(const ChatUser *chatUser) const
{
    if (!chatUser) {
        return nullptr;
    }
    return AddressBook::instance().lookupByChatUser(chatUser);
}
