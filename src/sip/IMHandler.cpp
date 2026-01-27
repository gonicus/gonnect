#include <QLoggingCategory>
#include <QRegularExpression>
#include <QUrl>
#include <QUuid>
#include <QDesktopServices>
#include <pjsua-lib/pjsua.h>
#include <qurlquery.h>

#include "SIPCall.h"
#include "SIPAccount.h"
#include "UserInfo.h"
#include "IMHandler.h"
#include "ViewHelper.h"

Q_LOGGING_CATEGORY(lcIMHandler, "gonnect.sip.im")

IMHandler::IMHandler(SIPCall *parent) : QObject(parent), m_call(parent)
{
    ReadOnlyConfdSettings settings;

    if (settings.contains("jitsi/url")) {

        settings.beginGroup("jitsi");
        m_jitsiBaseURL = settings.value("url", "").toString();
        m_jitsiPreconfig = settings.value("preconfig", false).toBool();

        settings.endGroup();
    }
}

bool IMHandler::process(const QString &contentType, const QString &message)
{
    if (contentType != "application/x-www-form-urlencoded") {
        qCWarning(lcIMHandler) << "content type" << contentType << "is not supported";
        return false;
    }

    QUrl callUrl = message;
    if (!callUrl.isValid() || callUrl.scheme() != "gonnect") {
        qCWarning(lcIMHandler) << "invalid URL received:" << message;
        return false;
    }

    // Read capabilites
    QString path = callUrl.path();

    if (path == "capabilities") {
        m_capabilities.clear();

        QUrlQuery q(callUrl);
        auto qitems = q.queryItems();

        for (auto &qi : std::as_const(qitems)) {
            if (qi.second == "1") {
                m_capabilities.push_back(qi.first);
            }
        }

        qCDebug(lcIMHandler) << "call party announced capabilities:" << m_capabilities;

        Q_EMIT capabilitiesChanged();
    }

    // Jitsi handler
    else if (path == "jitsi" && jitsiEnabled()) {
        QString meetingId;
        QString displayName;
        QUrlQuery q(callUrl);
        auto qitems = q.queryItems();

        for (auto &qi : std::as_const(qitems)) {
            if (qi.first == "meetingId") {
                meetingId = qi.second;
            }
            if (qi.first == "displayName") {
                displayName = qi.second;
            }
        }

        if (meetingId.isEmpty()) {
            qCWarning(lcIMHandler) << "invalid URL received: missing meetingId";
            return false;
        }

        m_jistiRequestedMeetingId = meetingId;
        Q_EMIT meetingRequested(m_call->account()->id(), m_call->getId());

        return true;
    }

    // Call delays
    else if (path == "callDelay") {
        bool validTimestamp = false;
        qint64 timestamp;
        QString digit;
        QUrlQuery q(callUrl);
        auto qitems = q.queryItems();

        for (auto &qi : std::as_const(qitems)) {
            if (qi.first == "timestamp") {
                timestamp = qi.second.toLongLong(&validTimestamp);
            }
            if (qi.first == "digit") {
                digit = qi.second;
            }
        }

        if (!validTimestamp) {
            qCWarning(lcIMHandler) << "invalid call delay timestamp received";
            return false;
        }

        m_call->initializeCallDelay(timestamp, digit);

        return true;
    }

    return false;
}

void IMHandler::openMeeting(const QString &meetingId, const QString &displayName, bool hangup,
                            QPointer<CallHistoryItem> callHistoryItem)
{
    // For whatever reason this needs to be decoupled to make QDesktopServices work
    QTimer::singleShot(0, this, [this, hangup, meetingId, callHistoryItem, displayName]() {
        ViewHelper::instance().requestMeeting(meetingId, callHistoryItem, displayName);

        if (hangup) {
            QTimer::singleShot(200, this, [this]() { m_call->account()->hangup(m_call->getId()); });
        }
    });
}

bool IMHandler::requestMeeting(bool hangup, QPointer<CallHistoryItem> callHistoryItem,
                               const QString &displayName)
{
    if (!m_capabilities.contains("jitsi")) {
        return false;
    }

    pj::SendInstantMessageParam prm;
    prm.contentType = "application/x-www-form-urlencoded";

    QString meetingId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QUrlQuery q;
    q.addQueryItem("meetingId", meetingId);
    q.addQueryItem("displayName", displayName);

    QUrl jitsiUrl("gonnect:");
    jitsiUrl.setPath("jitsi");
    jitsiUrl.setQuery(q);
    prm.content = jitsiUrl.toString().toStdString();

    try {
        m_call->sendInstantMessage(prm);
    } catch (pj::Error &err) {
        qCWarning(lcIMHandler) << "failed to send jitsi invitation:" << err.info();
        return false;
    }

    openMeeting(meetingId, displayName, hangup, callHistoryItem);

    return true;
}

bool IMHandler::sendCapabilities()
{
    if (--m_capabilitySendingTries == 0) {
        return false;
    }

    pj::SendInstantMessageParam prm;
    prm.contentType = "application/x-www-form-urlencoded";

    m_ownCapabilities.clear();

    QUrlQuery q;
    if (!m_jitsiBaseURL.isEmpty()) {
        q.addQueryItem("jitsi", "1");
        m_ownCapabilities.push_back("jitsi");
    }

    QUrl jitsiUrl("gonnect:");
    jitsiUrl.setPath("capabilities");
    jitsiUrl.setQuery(q);
    prm.content = jitsiUrl.toString().toStdString();

    try {
        m_call->sendInstantMessage(prm);
        m_capabilitiesSent = true;
    } catch (pj::Error &err) {
        qCWarning(lcIMHandler) << "failed to send capabilities:" << err.info();
        return false;
    }

    Q_EMIT capabilitiesChanged();
    return true;
}

bool IMHandler::triggerCapability(const QString &capability,
                                  QPointer<CallHistoryItem> callHistoryItem)
{
    if (capability == "jitsi:hangup") {
        return requestMeeting(true, callHistoryItem, tr("Ad hoc conference"));
    } else if (capability == "jitsi") {
        return requestMeeting(false, callHistoryItem, tr("Ad hoc conference"));
    } else if (capability == "jitsi:openMeeting") {
        openMeeting(m_jistiRequestedMeetingId, tr("Ad hoc conference"), false, callHistoryItem);
        return true;
    } else if (capability == "jitsi:openMeeting:hangup") {
        openMeeting(m_jistiRequestedMeetingId, tr("Ad hoc conference"), true, callHistoryItem);
        return true;
    }

    return false;
}

IMHandler::~IMHandler() { }
