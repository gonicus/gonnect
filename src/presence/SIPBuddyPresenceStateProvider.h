#pragma once

#include "IPresenceStateProvider.h"

class SIPBuddy;

class SIPBuddyPresenceStateProvider : public IPresenceStateProvider
{
    Q_OBJECT

public:
    explicit SIPBuddyPresenceStateProvider(const QString &sipUrl, QObject *parent = nullptr);

private Q_SLOTS:
    void updateState();

private:
    QString m_sipUrl;
};
