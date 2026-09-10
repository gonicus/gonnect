#pragma once

#include "SIPCallRoutingHop.h"
#include "Contact.h"

#include <QPointer>

struct CallRoutingHopInfo : public SIPCallRoutingHop
{
    explicit CallRoutingHopInfo(const SIPCallRoutingHop &other) : SIPCallRoutingHop(other) { }

    QString contactName() const { return contact ? contact->name() : ""; }

    QString phoneNumber;
    QPointer<Contact> contact = nullptr;
};
