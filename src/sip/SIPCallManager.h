#pragma once
#include <QObject>
#include <QTimer>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

#include "AppSettings.h"
#include "SIPCall.h"

class DtmfGenerator;

class SIPCallManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConferenceMode READ isConferenceMode NOTIFY isConferenceModeChanged FINAL)
    Q_PROPERTY(quint8 establishedCallsCount READ establishedCallsCount NOTIFY
                       establishedCallsCountChanged FINAL)
    Q_PROPERTY(unsigned missedCalls READ missedCalls NOTIFY missedCallsChanged FINAL)
    Q_DISABLE_COPY(SIPCallManager)

public:
    Q_REQUIRED_RESULT static SIPCallManager &instance()
    {
        static SIPCallManager *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new SIPCallManager();
        }

        return *_instance;
    }

    void initialize();
    void initBridge();

    void addMetadata(const QString &id, const QString &data);

    quint8 establishedCallsCount() const { return m_establishedCallsCount; }
    bool hasEstablishedCalls() const { return m_hasEstablishedCalls; }
    unsigned activeCalls() const { return m_activeCalls; }

    bool isEarlyCallState() const { return m_earlyCallState; }

    unsigned missedCalls() const { return m_missedCalls; }

    QStringList callIds() const;
    Q_INVOKABLE QString call(const QString &number, bool silent = false);
    Q_INVOKABLE QString call(const QString &accountId, const QString &number,
                             const QString &contactId = "",
                             const QString &preferredIdentity = "auto", bool silent = false);
    Q_INVOKABLE void endCall(const QString &accountId, const int callId);
    Q_INVOKABLE void endCall(SIPCall *call);
    Q_INVOKABLE void endCall(QString id);
    Q_INVOKABLE void endAllCalls();
    void holdOtherCalls(const SIPCall *call);
    void holdAllCalls() const;
    void unholdAllCalls() const;
    Q_INVOKABLE void holdCall(const QString &accountId, const int callId);
    Q_INVOKABLE void unholdCall(const QString &accountId, const int callId);
    Q_INVOKABLE void acceptCall(const QString &accountId, const int callId);
    void acceptCall(SIPCall *call);
    void rejectCall(SIPCall *call);

    void terminateAllNonEmergencyCalls();

    /// Transfer one call ("from") to another one ("to")
    Q_INVOKABLE void transferCall(const QString &fromAccountId, int fromCallId,
                                  const QString &toAccountId, int toCallId);

    SIPCall *findCall(const QString &accountId, int callId) const;
    SIPCall *findCall(const QString &remoteUri) const;
    SIPCall *findCallById(const QString &id) const;

    Q_INVOKABLE void triggerCapability(const QString &accountId, const int callId,
                                       const QString &capability) const;
    Q_INVOKABLE void startConference();
    Q_INVOKABLE void endConference();
    bool isConferenceMode() const { return m_isConferenceMode; }

    void toggleHold();
    bool isOneCallOnHold() const;

    Q_INVOKABLE void sendDtmf(const QString &accountId, const int callId, const QString &digit);
    Q_INVOKABLE void resetMissedCalls();

    /// Blocks the contact temporarily, i.e. there will be no notifications about incoming calls.
    /// When a valid contactId is given, all numbers of the contact will be blocked and the
    /// phoneNumber parameter will be ignored. Otherwise, just the phoneNumber will be blocked.
    Q_INVOKABLE void toggleTemporaryBlock(const QString &contactId, const QString &phoneNumber);

    bool isContactBlocked(const QString &contactId) const;
    bool isPhoneNumberBlocked(const QString &phoneNumber) const;
    bool beBusyOnNextIncomingCall() const;

    void addCall(SIPCall *call);
    void removeCall(SIPCall *call);
    void updateCallCount();

    QList<SIPCall *> calls() const { return m_calls; }

    ~SIPCallManager() = default;

signals:
    void incomingCall(SIPCall *call);
    void callsChanged();
    void callAdded(QString accountId, int callId);
    void callState(int callId, int statusCode);
    void establishedCallsCountChanged();
    void activeCallsChanged();
    void missedCallsChanged();
    void earlyCallStateChanged();
    void establishedChanged(SIPCall *call);
    void isHoldingChanged(SIPCall *call);
    void isConferenceModeChanged();
    void callContactChanged(SIPCall *call);
    void metadataChanged(SIPCall *call);
    void capabilitiesChanged(SIPCall *call);
    void audioLevelChanged(SIPCall *call, qreal level);
    void showCallWindow();
    void blocksChanged();
    void isBlockedChanged(SIPCall *call);

private slots:
    void dispatchDtmfBuffer();
    void cleanupBlocks();
    void updateBlockTimerRunning();

private:
    SIPCallManager(QObject *parent = nullptr);

    void onIncomingCall(SIPCall *call);

    AppSettings m_settings;
    DtmfGenerator *m_dtmfGen = nullptr;
    QList<SIPCall *> m_calls;
    QTimer m_dtmfTimer;
    QHash<std::pair<QString, int>, QString> m_dtmfBuffer;

    QTimer m_blockCleanTimer;
    QList<QPair<QDateTime, QString>> m_tempBlockedNumbers;
    QList<QPair<QDateTime, QString>> m_tempBlockedContacts;

    unsigned m_activeCalls = 0;
    unsigned m_missedCalls = 0;
    quint8 m_establishedCallsCount = 0;

    float m_micGainValue = 1.0;

    bool m_isConferenceMode = false;
    bool m_hasEstablishedCalls = false;
    bool m_earlyCallState = false;
    bool m_bridgeConfigured = false;
};

class SIPCallManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(SIPCallManager)
    QML_NAMED_ELEMENT(SIPCallManager)
    QML_SINGLETON

public:
    static SIPCallManager *create(QQmlEngine *, QJSEngine *) { return &SIPCallManager::instance(); }

private:
    SIPCallManagerWrapper() = default;
};
