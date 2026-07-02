#pragma once

#include "SIPCallRoutingHop.h"
#include "Contact.h"

#include <qqmlintegration.h>
#include <QPointer>

struct CallRoutingHopInfo : public SIPCallRoutingHop
{
    Q_GADGET
    QML_STRUCTURED_VALUE

    Q_PROPERTY(QString uri MEMBER uri CONSTANT FINAL)
    Q_PROPERTY(QString reasonText MEMBER reasonText CONSTANT FINAL)
    Q_PROPERTY(QString phoneNumber MEMBER phoneNumber CONSTANT FINAL)
    Q_PROPERTY(QString contactName READ contactName CONSTANT FINAL)

public:
    explicit CallRoutingHopInfo(const SIPCallRoutingHop &other) : SIPCallRoutingHop(other) { }

    QString contactName() const { return contact ? contact->name() : ""; }

    QString phoneNumber;
    QPointer<Contact> contact = nullptr;
};
