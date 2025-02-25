#include <QtDBus/QDBusConnection>
#include <QLoggingCategory>

#include "MediaPlayerInterface.h"
#include "ExternalMediaManager.h"

#define MPRIS_CALL_TIMEOUT 2000

Q_LOGGING_CATEGORY(lcExternalMedia, "gonnect.external.media")

ExternalMediaManager::ExternalMediaManager(QObject *parent) : QObject(parent) { }

ExternalMediaManager::~ExternalMediaManager() { }

void ExternalMediaManager::pause()
{
    auto con = QDBusConnection::sessionBus();
    if (!con.isConnected()) {
        return;
    }

    if (m_playerActiveTargets.isEmpty()) {
        auto services = getMprisTargets();
        for (auto &service : std::as_const(services)) {
            if (service.startsWith("org.mpris.MediaPlayer2.")) {
                OrgMprisMediaPlayer2PlayerInterface mediaInterface(
                        service, "/org/mpris/MediaPlayer2", con, this);
                mediaInterface.setTimeout(MPRIS_CALL_TIMEOUT);

                if (mediaInterface.playbackStatus() == "Playing") {
                    qCDebug(lcExternalMedia) << "pausing" << service;
                    m_playerActiveTargets.append(service);
                    auto res = mediaInterface.Pause();
                    if (res.isError()) {
                        qCWarning(lcExternalMedia)
                                << "failed to pause media player:" << res.error();
                    }
                }
            }
        }
    }
}

void ExternalMediaManager::resume()
{
    auto con = QDBusConnection::sessionBus();
    if (!con.isConnected()) {
        return;
    }

    for (auto &service : std::as_const(m_playerActiveTargets)) {
        OrgMprisMediaPlayer2PlayerInterface mediaInterface(service, "/org/mpris/MediaPlayer2", con,
                                                           this);
        mediaInterface.setTimeout(MPRIS_CALL_TIMEOUT);

        qCDebug(lcExternalMedia) << "resuming" << service;
        auto res = mediaInterface.Play();
        if (res.isError()) {
            qCWarning(lcExternalMedia) << "failed to resume media player:" << res.error();
        }
    }

    m_playerActiveTargets.clear();
}

QStringList ExternalMediaManager::getMprisTargets() const
{
    auto con = QDBusConnection::sessionBus();

    if (con.isConnected()) {
        QDBusMessage msg =
                QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                               "org.freedesktop.DBus", "ListNames");

        QDBusMessage reply = con.call(msg, QDBus::Block, MPRIS_CALL_TIMEOUT);
        if (!reply.arguments().isEmpty()) {
            return reply.arguments().at(0).toStringList();
        }
    }

    return {};
}
