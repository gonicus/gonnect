#pragma once

#include "AbstractPortal.h"
#include "PortalRequest.h"
#include "InhibitHelper.h"

#define INHIBIT_PORTAL_INTERFACE "org.freedesktop.portal.Inhibit"

class InhibitPortal : public AbstractPortal
{
    Q_OBJECT
    Q_DISABLE_COPY(InhibitPortal)

public:
    explicit InhibitPortal(QObject *parent = nullptr);
    ~InhibitPortal();

    void queryEndResponse();
    void inhibit(unsigned int flags, const QString &reason, PortalResponse callback);
    void release();

signals:
    void stateChanged(bool screensaverActive, InhibitHelper::InhibitState state);

public slots:
    void sessionStateReceived(const QDBusObjectPath &session_handle, const QVariantMap &map);

private:
    void createMonitor(PortalResponse callback);

    OrgFreedesktopPortalRequestInterface *m_request = nullptr;
};
