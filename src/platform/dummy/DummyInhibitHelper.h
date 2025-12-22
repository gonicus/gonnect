#pragma once
#include <QObject>
#include "../InhibitHelper.h"

class DummyInhibitHelper : public InhibitHelper
{
    Q_OBJECT

public:
    explicit DummyInhibitHelper() : InhibitHelper() { }

    void inhibit(unsigned int flags, const QString &reason) override
    {
        Q_UNUSED(flags);
        Q_UNUSED(reason);
    }
    void release() override { }

    void inhibitScreenSaver(const QString &applicationName, const QString &reason) override
    {
        Q_UNUSED(applicationName);
        Q_UNUSED(reason);
    }
    void releaseScreenSaver() override { }
};
