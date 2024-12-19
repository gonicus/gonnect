#pragma once
#include <QObject>

class AccountPortal;
class SIPCall;

class IMHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(IMHandler)

public:
    explicit IMHandler(SIPCall *parent);
    ~IMHandler();

    bool jitsiEnabled() const
    {
        return !m_jitsiBaseURL.isEmpty() && m_capabilities.contains("jitsi");
    }
    bool process(const QString &contentType, const QString &message);

    bool capabilitiesSent() const { return m_capabilitiesSent; }
    bool sendCapabilities();
    bool triggerCapability(const QString &capability);

    bool hasCapability(const QString &capability) const
    {
        return m_capabilities.contains(capability) && m_ownCapabilities.contains(capability);
    }

    void openMeeting(const QString &meetingId, bool hangup = false);

signals:
    void capabilitiesChanged();
    void meetingRequested(const QString &accountId, int callId);

private:
    void acquireDisplayName(std::function<void(const QString &displayName)> callback);
    bool requestMeeting();

    AccountPortal *m_accountPortal = nullptr;
    SIPCall *m_call = nullptr;

    QStringList m_capabilities;
    QStringList m_ownCapabilities;
    QString m_jitsiBaseURL;
    QString m_jitsiDisplayName;
    QString m_jistiRequestedMeetingId;

    unsigned m_capabilitySendingTries = 3;

    bool m_capabilitiesSent = false;
    bool m_jitsiPreconfig = false;
};
