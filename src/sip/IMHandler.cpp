#include <QLoggingCategory>
#include <QRegularExpression>
#include <QUrl>
#include <QDesktopServices>
#include <pjsua-lib/pjsua.h>

#include "AppSettings.h"
#include "SIPCall.h"
#include "SIPAccount.h"
#include "AccountPortal.h"
#include "IMHandler.h"

Q_LOGGING_CATEGORY(lcIMHandler, "gonnect.sip.im")

IMHandler::IMHandler(SIPCall *parent) : QObject(parent), m_call(parent)
{
    ReadOnlyConfdSettings settings;

    if (settings.contains("jitsi/url")) {
        m_accountPortal = new AccountPortal(this);

        settings.beginGroup("jitsi");
        m_jitsiBaseURL = settings.value("url", "").toString();
        m_jitsiPreconfig = settings.value("preconfig", false).toBool();
        m_jitsiDisplayName = settings.value("displayName", "").toString();

        settings.endGroup();
    }
}

void IMHandler::acquireDisplayName(std::function<void(const QString &displayName)> callback)
{
    if (m_jitsiDisplayName.isEmpty()) {
        m_accountPortal->GetUserInformation(
                tr("The VoIP phone wants to use your name to pre set the display name in Jitsi."),
                [this, callback](uint code, const QVariantMap &response) {
                    QString username;

                    if (code == 0) {
                        username = response.value("name", "").toString();
                        m_jitsiDisplayName = username;
                        if (!username.isEmpty()) {
                            AppSettings settings;
                            settings.setValue("jitsi/displayName", username);
                        }
                    } else {
                        m_jitsiPreconfig = false;
                    }

                    callback(m_jitsiDisplayName);
                });

        return;
    }

    callback(m_jitsiDisplayName);
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

        emit capabilitiesChanged();
    }

    // Jitsi handler
    else if (path == "jitsi" && jitsiEnabled()) {
        QString meetingId;
        QUrlQuery q(callUrl);
        auto qitems = q.queryItems();

        for (auto &qi : std::as_const(qitems)) {
            if (qi.first == "meetingId") {
                meetingId = qi.second;
            }
        }

        if (meetingId.isEmpty()) {
            qCWarning(lcIMHandler) << "invalid URL received: missing meetingId";
            return false;
        }

        m_jistiRequestedMeetingId = meetingId;
        emit meetingRequested(m_call->account()->id(), m_call->getId());

        return true;
    }

    return false;
}

void IMHandler::openMeeting(const QString &meetingId, bool hangup)
{
    acquireDisplayName([this, meetingId, hangup](const QString &displayName) {
        QUrlQuery q;
        q.addQueryItem("userInfo.displayName", displayName);
        q.addQueryItem("config.prejoinConfig.enabled", m_jitsiPreconfig ? "true" : "false");

        QUrl jitsiUrl(m_jitsiBaseURL);
        jitsiUrl.setPath("/" + meetingId);
        jitsiUrl.setQuery(q);

        // For whatever reason this needs to be decoupled to make QDesktopServices work
        QTimer::singleShot(0, this, [this, hangup, jitsiUrl]() {
            QDesktopServices::openUrl(jitsiUrl);

            if (hangup) {
                m_call->account()->hangup(m_call->getId());
            }
        });
    });
}

bool IMHandler::requestMeeting()
{
    if (!m_capabilities.contains("jitsi")) {
        return false;
    }

    pj::SendInstantMessageParam prm;
    prm.contentType = "application/x-www-form-urlencoded";

    QString meetingId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QUrlQuery q;
    q.addQueryItem("meetingId", meetingId);

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

    openMeeting(meetingId);

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

    emit capabilitiesChanged();
    return true;
}

bool IMHandler::triggerCapability(const QString &capability)
{
    if (capability == "jitsi") {
        return requestMeeting();
    }

    else if (capability == "jitsi:openMeeting") {
        openMeeting(m_jistiRequestedMeetingId);
        return true;
    }

    else if (capability == "jitsi:openMeeting:hangup") {
        openMeeting(m_jistiRequestedMeetingId, true);
        return true;
    }

    return false;
}

IMHandler::~IMHandler() { }
