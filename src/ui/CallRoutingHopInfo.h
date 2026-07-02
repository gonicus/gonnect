#pragma once

#include "SIPCallRoutingHop.h"
#include "Contact.h"

#include <qqmlintegration.h>

struct CallRoutingHopInfo : public SIPCallRoutingHop
{
    Q_GADGET
    QML_STRUCTURED_VALUE

    Q_PROPERTY(QString uri MEMBER uri CONSTANT FINAL)
    Q_PROPERTY(QString reasonText MEMBER reasonText CONSTANT FINAL)
    Q_PROPERTY(QString phoneNumber MEMBER phoneNumber CONSTANT FINAL)
    Q_PROPERTY(Contact *contact MEMBER contact CONSTANT FINAL)

public:
    explicit CallRoutingHopInfo(const SIPCallRoutingHop &other) : SIPCallRoutingHop(other) { }

    QString phoneNumber;
    Contact *contact = nullptr;
};
