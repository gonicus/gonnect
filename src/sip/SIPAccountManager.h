#pragma once
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

    SIPAccount *getAccount(const QString &accountId);
    QList<SIPAccount *> accounts() const { return m_accounts; }

    Q_INVOKABLE void setAccountCredentials(const QString &accountId, const QString &password);

    ~SIPAccountManager() = default;

    void initialize();

signals:
    void accountsChanged();
    void sipRegisteredChanged();
    void authorizationFailed(QString accountId);

private:
    SIPAccountManager(QObject *parent = nullptr);
    void setSipRegistered(bool value);
    void updateSipRegistered();

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
        return &SIPAccountManager::instance();
    }

private:
    SIPAccountManagerWrapper() = default;
};
