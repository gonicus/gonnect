#pragma once
#include <QObject>
#include <QAbstractNativeEventFilter>
#include "../InhibitHelper.h"

class WindowsInhibitHelper : public InhibitHelper
{
    Q_OBJECT

public:
    explicit WindowsInhibitHelper();

    void inhibit(unsigned int flags, const QString &reason) override;
    void release() override;

    void inhibitScreenSaver(const QString &applicationName, const QString &reason) override;
    void releaseScreenSaver() override;

    bool inhibitActive() const override;

private:
    bool m_screenSaverIsInhibited = false;
    bool m_inhibit = false;
};

class WindowsEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long long *) override;
};
