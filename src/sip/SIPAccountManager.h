#pragma once

#include <QQmlEngine>
#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include "ReadOnlyConfdSettings.h"
#include "SIPAccount.h"

class SIPAccountManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SIPAccountManager)

    Q_PROPERTY(bool sipRegistered READ sipRegistered NOTIFY sipRegisteredChanged FINAL)
    Q_PROPERTY(uint sipRegisterRetryInterval READ sipRegisterRetryInterval CONSTANT FINAL)
    Q_PROPERTY(qint16 newVoiceMessageCount READ newVoiceMessageCount NOTIFY
                       voiceMessagesWaitingChanged FINAL)
    Q_PROPERTY(qint16 oldVoiceMessageCount READ oldVoiceMessageCount NOTIFY
                       voiceMessagesWaitingChanged FINAL)

public:
    Q_REQUIRED_RESULT static SIPAccountManager &instance()
    {
        static SIPAccountManager *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new SIPAccountManager();
        }

        return *_instance;
    }

    bool sipRegistered() const { return m_sipRegistered; }
    uint sipRegisterRetryInterval() const;
    qint16 newVoiceMessageCount() const;
    qint16 oldVoiceMessageCount() const;

    SIPAccount *getAccount(const QString &accountId);
    QList<SIPAccount *> accounts() const { return m_accounts; }

    Q_INVOKABLE void setAccountCredentials(const QString &accountId, const QString &password);
    Q_INVOKABLE void callVoiceBox(const QString &accountId);

    ~SIPAccountManager() = default;

    void initialize();

    bool hasConfiguration() const { return m_numberOfAccounts; }

Q_SIGNALS:
    void accountsChanged();
    void sipRegisteredChanged(bool status);
    void voiceMessagesWaitingChanged();
    void authorizationFailed(QString accountId);
    void connectionError(int code, QString message);

private:
    SIPAccountManager(QObject *parent = nullptr);
    void setSipRegistered(bool value);
    void updateSipRegistered();

    unsigned m_numberOfAccounts = 0;
    int m_lastErrorCode = 0;

    QList<SIPAccount *> m_accounts;
    bool m_sipRegistered = false;
    ReadOnlyConfdSettings m_settings;
};

class SIPAccountManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(SIPAccountManager)
    QML_NAMED_ELEMENT(SIPAccountManager)
    QML_SINGLETON

public:
    static SIPAccountManager *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&SIPAccountManager::instance(), QQmlEngine::CppOwnership);
        return &SIPAccountManager::instance();
    }

private:
    SIPAccountManagerWrapper() = default;
};
