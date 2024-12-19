#pragma once

#include "AbstractPortal.h"

#define ACCOUNT_PORTAL_INTERFACE "org.freedesktop.portal.Account"

class AccountPortal : public AbstractPortal
{
    Q_OBJECT
    Q_DISABLE_COPY(AccountPortal)

public:
    explicit AccountPortal(QObject *parent = nullptr);
    ~AccountPortal() = default;

    void GetUserInformation(const QString &reason, PortalResponse callback);
};
