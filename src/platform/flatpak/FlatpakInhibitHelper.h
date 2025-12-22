#pragma once
#include <QObject>
#include "../InhibitHelper.h"

class InhibitPortal;
class OrgFreedesktopScreenSaverInterface;

class FlatpakInhibitHelper : public InhibitHelper
{
    Q_OBJECT

public:
    explicit FlatpakInhibitHelper();

    void inhibit(unsigned int flags, const QString &reason) override;
    void release() override;

    void inhibitScreenSaver(const QString &applicationName, const QString &reason) override;
    void releaseScreenSaver() override;

private:
    void queryEndResponse();

    InhibitPortal *m_portal = nullptr;
    OrgFreedesktopScreenSaverInterface *m_screenSaverInterface = nullptr;

    unsigned m_screenSaverCookie = 0;
    bool m_screenSaverIsInhibited = false;
};
