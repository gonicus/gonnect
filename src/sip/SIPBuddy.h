#pragma once
#include <QObject>
#include <QTimer>
#include <pjsua2.hpp>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

class SIPAccount;

class SIPBuddyState
{
    Q_GADGET

public:
    enum STATUS { UNKNOWN = 0, UNAVAILABLE, READY, RINGING, BUSY };
    Q_ENUM(STATUS)
};

namespace SIPBuddyStateWrapper {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(SIPBuddyState)
QML_NAMED_ELEMENT(SIPBuddyState)
}; // namespace SIPBuddyStateWrapper

class SIPBuddy : public QObject, public pj::Buddy
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("This object is managed in C++")
    Q_DISABLE_COPY(SIPBuddy)

public:
    explicit SIPBuddy(SIPAccount *account, const QString &uri);
    virtual ~SIPBuddy();

    bool initialize();

    virtual void onBuddyState();

    SIPAccount *account() const { return m_account; };

    QString uri() const { return m_uri; }

    QString statusText() const { return m_statusText; }
    SIPBuddyState::STATUS status() const { return m_status; }

    Q_INVOKABLE bool isSubscribedToStatus() { return m_subscribeToStatus; };
    Q_INVOKABLE void subscribeToBuddyStatus();
    void notifyOnceWhenBuddyAvailable();

signals:
    void statusChanged(SIPBuddyState::STATUS);

private:
    QString m_statusText;
    QString m_uri;

    SIPBuddyState::STATUS m_status;

    SIPAccount *m_account = nullptr;

    bool m_subscribeToStatus = false;
    QTimer m_subscribeTimeout;
};
