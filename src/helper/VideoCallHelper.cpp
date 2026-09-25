#include "VideoCallHelper.h"
#include "GlobalCallState.h"
#include "IConferenceConnector.h"
#include "ViewHelper.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcVideoCallHelper, "gonnect.app.helper.VideoCallHelper")

VideoCallHelper::VideoCallHelper(QObject *parent) : QObject{ parent }
{
    connect(&GlobalCallState::instance(), &GlobalCallState::callStarted, this,
            [this](bool isConference) {
                if (isConference) {
                    updateActiveVideoCall();
                }
            });
    connect(&GlobalCallState::instance(), &GlobalCallState::callEnded, this,
            [this](bool isConference) {
                if (isConference) {
                    updateActiveVideoCall();
                }
            });
    updateActiveVideoCall();
}

QUrl VideoCallHelper::normalizeUrl(const QUrl &url) const
{
    const auto str = url.toString(QUrl::FullyEncoded).trimmed();
    auto normUrl = QUrl::fromEncoded(str.toUtf8(), QUrl::TolerantMode);
    normUrl = url.adjusted(QUrl::NormalizePathSegments);
    return normUrl;
}

bool VideoCallHelper::isPrefix(const QUrl &url, const QUrl &possiblePrefix) const
{
    const auto prefix = normalizeUrl(possiblePrefix);
    const auto full = normalizeUrl(url);

    if (prefix.scheme().compare(full.scheme(), Qt::CaseInsensitive) != 0) {
        return false;
    }
    if (prefix.host().compare(full.host(), Qt::CaseInsensitive) != 0) {
        return false;
    }
    if (prefix.port(-1) != -1 && prefix.port(-1) != full.port(-1)) {
        return false;
    }
    if (prefix.userName() != full.userName() || prefix.password() != full.password()) {
        return false;
    }

    const auto segPrefix = prefix.path(QUrl::FullyDecoded).split('/', Qt::SkipEmptyParts);
    const auto segFull = full.path(QUrl::FullyDecoded).split('/', Qt::SkipEmptyParts);

    if (segPrefix.size() > segFull.size()) {
        return false;
    }

    for (qsizetype i = 0; i < segPrefix.size(); ++i) {
        if (segPrefix.at(i) != segFull.at(i)) {
            return false;
        }
    }

    return true;
}

QString VideoCallHelper::remainderPath(const QUrl &url, const QUrl &prefix) const
{
    const auto segPrefix =
            normalizeUrl(prefix).path(QUrl::FullyDecoded).split('/', Qt::SkipEmptyParts);
    const auto segFull = normalizeUrl(url).path(QUrl::FullyDecoded).split('/', Qt::SkipEmptyParts);

    return segFull.mid(segPrefix.size()).join('/');
}

IConferenceConnector *VideoCallHelper::matchingConferenceConnector(const QString &url) const
{
    if (url.isEmpty()) {
        return nullptr;
    }

    const QUrl u(url);
    if (!u.isValid()) {
        return nullptr;
    }

    const auto &callStateObjects = GlobalCallState::instance().globalCallStateObjects();

    for (auto *callStateObject : callStateObjects) {
        if (auto *conferenceConn = qobject_cast<IConferenceConnector *>(callStateObject)) {
            if (isPrefix(u, conferenceConn->baseUrl())) {
                return conferenceConn;
            }
        }
    }

    return nullptr;
}

void VideoCallHelper::updateActiveVideoCall()
{

    QString newConferenceUrl;

    const auto &callStateObjects = GlobalCallState::instance().globalCallStateObjects();

    for (const auto *callStateObject : callStateObjects) {
        if (const auto *conferenceConn =
                    qobject_cast<const IConferenceConnector *>(callStateObject)) {
            if (conferenceConn->isInConference()) {
                newConferenceUrl = conferenceConn->conferenceUrl().toString();
                break;
            }
        }
    }

    if (m_activeVideoCallUrl != newConferenceUrl) {
        const bool hasActiveChanged = m_activeVideoCallUrl.isEmpty() != newConferenceUrl.isEmpty();

        m_activeVideoCallUrl = newConferenceUrl;
        Q_EMIT activeVideoCallUrlChanged();

        if (hasActiveChanged) {
            Q_EMIT hasActiveVideoCallChanged();
        }
    }
}

void VideoCallHelper::joinOrStartConfernece(const QString &url) const
{
    if (auto *connector = matchingConferenceConnector(url)) {

        // TODO: Once multiple conference connectors are available, start directly on the correct
        // one.

        const auto roomFromUrl = remainderPath(url, connector->baseUrl());
        ViewHelper::instance().requestMeeting(roomFromUrl);
    } else {
        qCWarning(lcVideoCallHelper)
                << "Cannot join/start conference: no matching connector found for url" << url;
    }
}

bool VideoCallHelper::hasMatchingConferenceConnector(const QString &url) const
{
    return matchingConferenceConnector(url);
}
