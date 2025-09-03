#include "FlatpakInhibitHelper.h"
#include "InhibitHelper.h"
#include "InhibitPortal.h"
#include "ScreenSaverInterface.h"

InhibitHelper &InhibitHelper::instance()
{
    static InhibitHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new FlatpakInhibitHelper;
    }
    return *_instance;
}

FlatpakInhibitHelper::FlatpakInhibitHelper() : InhibitHelper{}
{
    m_portal = new InhibitPortal(this);

    connect(m_portal, &InhibitPortal::stateChanged, this,
            [this](bool screensaverActive, InhibitHelper::InhibitState state) {
                emit stateChanged(screensaverActive, state);
            });

    m_screenSaverInterface = new OrgFreedesktopScreenSaverInterface("org.freedesktop.ScreenSaver",
                                                                    "/org/freedesktop/ScreenSaver",
                                                                    QDBusConnection::sessionBus());
}

void FlatpakInhibitHelper::queryEndResponse()
{
    m_portal->queryEndResponse();
}

void FlatpakInhibitHelper::inhibit(unsigned int flags, const QString &reason)
{
    m_portal->inhibit(flags, reason, [this](uint code, const QVariantMap &) {
        if (code != 0) {
            qCCritical(lcInhibit) << "failed to inhibit session: response code" << code;
        } else {
            m_portal->queryEndResponse();
        }
    });
}

void FlatpakInhibitHelper::release()
{
    m_portal->release();
}

void FlatpakInhibitHelper::inhibitScreenSaver(const QString &applicationName, const QString &reason)
{
    if (m_screenSaverInterface && !m_screenSaverIsInhibited) {
        QDBusPendingReply<unsigned> reply =
                m_screenSaverInterface->Inhibit(applicationName, reason);
        reply.waitForFinished();

        m_screenSaverCookie = reply.value();
        m_screenSaverIsInhibited = true;
    }
}

void FlatpakInhibitHelper::releaseScreenSaver()
{
    if (m_screenSaverInterface && m_screenSaverIsInhibited) {
        m_screenSaverInterface->UnInhibit(m_screenSaverCookie);
        m_screenSaverIsInhibited = false;
    }
}
