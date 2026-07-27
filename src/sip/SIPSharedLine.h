#pragma once
#include <QObject>
#include <QString>
#include <pjsua2.hpp>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

class SIPAccount;

// Watch a shared line on CUCM by listening for dialog event (RFC 4235)
class SIPSharedLine : public QObject, public pj::Buddy
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("This object is managed in C++")
    Q_DISABLE_COPY(SIPSharedLine)

public:
    enum class SubscriptionState { Idle, Pending, Active, NotConfigured, Failed };
    Q_ENUM(SubscriptionState)

    explicit SIPSharedLine(SIPAccount *account, const QString &uri);
    ~SIPSharedLine() override;

    bool initialize();

    void onBuddyState() override { }
    void onBuddyDlgEventState() override;
    void onBuddyEvSubDlgEventState(pj::OnBuddyEvSubStateParam &prm) override;

    SIPAccount *account() const { return m_account; }
    QString uri() const { return m_uri; }

    SubscriptionState subscriptionState() const { return m_subscriptionState; }

    bool isRemoteInUse() const { return m_remoteInUse; }

    QString remoteCallId() const { return m_remoteCallId; }
    QString remoteLocalTag() const { return m_remoteLocalTag; }
    QString remoteRemoteTag() const { return m_remoteRemoteTag; }

    QString joinHeaderValue() const;

Q_SIGNALS:
    void subscriptionStateChanged();
    void remoteInUseChanged();

private:
    void setSubscriptionState(SubscriptionState state);
    void clearRemoteDialog();

    QString m_uri;
    SIPAccount *m_account = nullptr;

    SubscriptionState m_subscriptionState = SubscriptionState::Idle;

    bool m_remoteInUse = false;
    QString m_remoteCallId;
    QString m_remoteLocalTag;
    QString m_remoteRemoteTag;
    QString m_remoteState;
    QString m_remoteDirection;
};
