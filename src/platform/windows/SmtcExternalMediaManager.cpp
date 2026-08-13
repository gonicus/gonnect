#include <QLoggingCategory>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "SmtcExternalMediaManager.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Control.h>
#include <iostream>

using namespace winrt::Windows::Media::Control;

Q_LOGGING_CATEGORY(lcExternalMedia, "gonnect.external.media")

// Dedicated thread for simplified GlobalSystemMediaTransportControlsSessionManager access.
// This allows us to use blocking API to obtain sessions.
class SmtcThread : public QThread
{
public:
    enum class Request { None, PauseSessions, ResumeSessions, Quit };

public:
    void setRequest(Request request);

protected:
    void run() override;
    void pauseSessions();
    void resumeSessions();

private:
    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    Request m_currentRequest = Request::None;
    QSet<winrt::hstring> m_pausedSessions;
};

ExternalMediaManager &ExternalMediaManager::instance()
{
    static ExternalMediaManager *_instance = nullptr;
    if (!_instance) {
        _instance = new SmtcExternalMediaManager;
    }

    return *_instance;
}

SmtcExternalMediaManager::SmtcExternalMediaManager() : m_thread(new SmtcThread)
{
    m_thread->start();
}

SmtcExternalMediaManager::~SmtcExternalMediaManager()
{
    m_thread->setRequest(SmtcThread::Request::Quit);
    delete m_thread;
}

void SmtcExternalMediaManager::pause()
{
    m_didPause = true;
    m_thread->setRequest(SmtcThread::Request::PauseSessions);
}

void SmtcExternalMediaManager::resume()
{
    m_didPause = false;
    m_thread->setRequest(SmtcThread::Request::ResumeSessions);
}

void SmtcThread::run()
{
    winrt::init_apartment(); // required once per thread
    while (true) {
        QMutexLocker lock(&m_mutex);
        switch (m_currentRequest) {
        case Request::Quit:
            return;
        case Request::PauseSessions:
            pauseSessions();
            break;
        case Request::ResumeSessions:
            resumeSessions();
            break;
        case Request::None:
            break;
        }
        m_currentRequest = Request::None;
        m_waitCondition.wait(&m_mutex);
    }
}

void SmtcThread::setRequest(Request request)
{
    QMutexLocker lock(&m_mutex);
    if (m_currentRequest != Request::Quit) {
        m_currentRequest = request;
    }
    m_waitCondition.wakeAll();
}

void SmtcThread::pauseSessions()
{
    auto sessions =
            GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetSessions();
    for (auto const &session : sessions) {
        auto controls = session.GetPlaybackInfo().Controls();
        if (controls.IsPauseEnabled()) {
            qCDebug(lcExternalMedia) << "pausing" << session.SourceAppUserModelId();
            m_pausedSessions.insert(session.SourceAppUserModelId());
            session.TryPauseAsync();
        }
    }
}

void SmtcThread::resumeSessions()
{
    if (m_pausedSessions.empty()) {
        return;
    }
    auto sessions =
            GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetSessions();
    for (auto const &session : sessions) {
        if (m_pausedSessions.contains(session.SourceAppUserModelId())) {
            auto controls = session.GetPlaybackInfo().Controls();
            if (controls.IsPlayEnabled()) {
                qCDebug(lcExternalMedia) << "resuming" << session.SourceAppUserModelId();
                session.TryPlayAsync();
            } else {
                qCDebug(lcExternalMedia)
                        << "cannot resume paused session" << session.SourceAppUserModelId();
            }
        }
    }
    m_pausedSessions.clear();
}
