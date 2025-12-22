#pragma once
#include <QObject>
#include "../InhibitHelper.h"

class WindowsInhibitHelper : public InhibitHelper
{
    Q_OBJECT

public:
    explicit WindowsInhibitHelper();

    void inhibit(unsigned int flags, const QString &reason) override {}
    void release() override {}

    void inhibitScreenSaver(const QString &applicationName, const QString &reason) override;
    void releaseScreenSaver() override;

private:
    bool m_screenSaverIsInhibited = false;
};
