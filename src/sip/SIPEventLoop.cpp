#include "SIPEventLoop.h"
#include "SIPManager.h"

SIPEventLoop::SIPEventLoop(QObject *parent) : QObject(parent)
{
    connect(&ticker, &QTimer::timeout, this, []() {
        auto &ep = SIPManager::instance().endpoint();
        ep.libHandleEvents(10);
    });

    ticker.start(50);
}

SIPEventLoop::~SIPEventLoop()
{
    ticker.stop();
}
