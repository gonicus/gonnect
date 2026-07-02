#pragma once

#include <QObject>
#include <QQmlEngine>

#include "CallRoutingHopInfo.h"
#include "SIPCall.h"

class CallRoutingHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit CallRoutingHelper(QObject *parent = nullptr);

    static QList<CallRoutingHopInfo> routingHopsForCall(const SIPCall &call);
    Q_INVOKABLE QVariantList routingHopsForCall(const QString &accountId, int callId) const;
};
