#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStack>

#include "ICallState.h"
#include "PhoneNumberUtil.h"

class Ringer;

class GlobalCallState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ICallState::States globalCallState READ globalCallState NOTIFY globalCallStateChanged
                       FINAL)
    Q_PROPERTY(ICallState *callInForeground READ callInForeground WRITE setCallInForeground NOTIFY
                       callInForegroundChanged FINAL)
    Q_PROPERTY(
            qsizetype activeCallsCount READ activeCallsCount NOTIFY activeCallsCountChanged FINAL)
    Q_PROPERTY(qsizetype nonIdleCallsCount READ nonIdleCallsCount NOTIFY nonIdleCallsCountChanged
                       FINAL)

public:
    static GlobalCallState &instance()
    {
        static GlobalCallState *_instance = nullptr;
        if (!_instance) {
            _instance = new GlobalCallState;
        }
        return *_instance;
    }

    inline const ICallState::States globalCallState() const { return m_globalCallState; }

    bool registerCallStateObject(ICallState *callStateObject);
    bool unregisterCallStateObject(ICallState *callStateObject);

    void setIsPhoneConference(bool flag);
    bool isPhoneConference() const { return m_isPhoneConference; }

    ContactInfo remoteContactInfo() const { return m_remoteContactInfo; }

    const QSet<ICallState *> &globalCallStateObjects() const { return m_globalCallStateObjects; }
    qsizetype activeCallsCount() const;
    qsizetype nonIdleCallsCount() const;

    void setCallInForeground(ICallState *call);
    ICallState *callInForeground() const { return m_callInForeground; }

    Q_INVOKABLE void triggerHold();

    Q_INVOKABLE void holdAllCalls(const ICallState *stateObjectToSkip = nullptr) const;
    Q_INVOKABLE void unholdOtherCall() const;
    Q_INVOKABLE void unholdAllCalls() const;

private Q_SLOTS:
    void updateGlobalCallState();
    void updateRinger();
    void onCallInForegroundChanged();
    void updateRemoteContactInfo();

private:
    explicit GlobalCallState(QObject *parent = nullptr);
    void setGlobalCallState(const ICallState::States state);
    void setRemoteContactInfo(const ContactInfo &info);
    QSet<ICallState *> filteredCallStateObjected(
            const ICallState::States mustFulfilAll = ICallState::States::fromInt(0),
            const ICallState::States mustFulfilOne = ICallState::States::fromInt(0)) const;

    ICallState::States m_globalCallState = ICallState::State::Idle;
    QSet<ICallState *> m_globalCallStateObjects;
    ICallState *m_callInForeground = nullptr;
    QObject *m_foregroundCallContext = nullptr;

    Ringer *m_ringer = nullptr;
    ContactInfo m_remoteContactInfo;

    bool m_isPhoneConference = false;

Q_SIGNALS:
    void globalCallStateChanged();
    void remoteContactInfoChanged();
    void callInForegroundChanged();
    void isPhoneConferenceChanged();
    void globalCallStateObjectsChanged();
    void activeCallsCountChanged();
    void nonIdleCallsCountChanged();
    void callStarted(bool isConference);
    void callEnded(bool isConference);
};

class GlobalCallStateWrapper
{
    Q_GADGET
    QML_FOREIGN(GlobalCallState)
    QML_NAMED_ELEMENT(GlobalCallState)
    QML_SINGLETON

public:
    static GlobalCallState *create(QQmlEngine *, QJSEngine *)
    {
        return &GlobalCallState::instance();
    }

private:
    GlobalCallStateWrapper() = default;
};
