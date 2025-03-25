#pragma once
#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include <pjsua2.hpp>

#include "SIPCall.h"
#include "SIPBuddy.h"
#include "ReadOnlyConfdSettings.h"

class SIPAccount : public QObject, public pj::Account
{
    Q_OBJECT
    Q_DISABLE_COPY(SIPAccount)

public:
    SIPAccount(const QString &account, QObject *parent = nullptr);

    enum TRANSPORT_TYPE { UDP, TCP, TLS };
    Q_ENUM(TRANSPORT_TYPE)

    enum TRANSPORT_NET { AUTO, IPv4, IPv6 };
    Q_ENUM(TRANSPORT_NET)

    bool initialize();

    virtual void onIncomingCall(pj::OnIncomingCallParam &prm) override;
    virtual void onRegState(pj::OnRegStateParam &prm) override;
    virtual void onSendRequest(pj::OnSendRequestParam &prm) override;

    bool isRegistered() const { return m_isRegistered; }
    bool isInstantMessagingAllowed() const;

    void call(const QString &number, const QString &contactId = "",
              const QString &preferredIdentity = "auto", bool silent = false);
    void hangup(const int callId);
    void hold(const int callId);
    void unhold(const int callId);
    void removeCall(const int callId);
    void removeCall(SIPCall *call);
    QString toSipUri(const QString &var) const;
    QList<SIPCall *> calls() const { return m_calls; }
    QList<SIPBuddy *> buddies() const { return m_buddies; };
    SIPCall *getCallById(const int callId);

    QString id() const { return m_account; }
    QString domain() const { return m_domain; }
    uint retryInterval() const;

    SIPBuddyState::STATUS buddyStatus(const QString &var);

    void setCredentials(const QString &password);

    ~SIPAccount();

private:
    void generatePreferredIdentityHeader(const QString &var, const QString &preferredIdentity,
                                         pj::CallOpParam &prm);
    bool hasAllowGrant(const QString &header, const QString &grant) const;

    QString addTransport(const QString &uri);

    QList<SIPCall *> m_calls;
    QList<SIPBuddy *> m_buddies;
    bool m_isRegistered = false;
    bool m_isInstantMessagingAllowed = false;
    bool m_shallNegotiateCapabilities = true;
    bool m_useInstantMessagingWithoutCheck = true;

    QString m_account;
    QString m_domain;
    QByteArray m_optionsRequestUuid;
    pj::AccountConfig m_accountConfig;
    pj::TransportConfig m_transportConfig;

    ReadOnlyConfdSettings m_settings;

    TRANSPORT_TYPE m_transportType = TRANSPORT_TYPE::TLS;
    TRANSPORT_NET m_transportNet = TRANSPORT_NET::AUTO;

signals:
    void isRegisteredChanged();
    void authorizationFailed();
};
