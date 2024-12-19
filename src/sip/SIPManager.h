#pragma once
#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include <pjsua2.hpp>

#include "AppSettings.h"
#include "PreferredIdentity.h"
#include "SIPBuddy.h"

class SIPMediaConfig;
class SIPUserAgentConfig;
class SIPEventLoop;

class SIPLogWriter : public pj::LogWriter
{
public:
    virtual void write(const pj::LogEntry &entry);
};

class SIPManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SIPManager)

    Q_PROPERTY(QList<PreferredIdentity *> preferredIdentities READ preferredIdentities NOTIFY
                       preferredIdentitiesChanged FINAL)
    Q_PROPERTY(QString defaultPreferredIdentity READ defaultPreferredIdentity WRITE
                       setDefaultPreferredIdentity NOTIFY defaultPreferredIdentityChanged FINAL)

public:
    Q_REQUIRED_RESULT static SIPManager &instance()
    {
        static SIPManager *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new SIPManager();
        }

        return *_instance;
    }

    pj::Endpoint &endpoint() { return m_ep; }

    ~SIPManager() = default;

    void initialize();
    void shutdown();

    void getPlaybackDevices();

    void setDefaultPreferredIdentity(const QString &value);
    void initializePreferredIdentities();

    QString defaultPreferredIdentity() const { return m_defaultPreferredIdentity; }
    QList<PreferredIdentity *> preferredIdentities() const { return m_preferredIdentities; }
    QList<PreferredIdentity *> enrolledPreferredIdentities() const
    {
        return m_enrolledPreferredIdentities;
    }
    Q_INVOKABLE PreferredIdentity *addEmptyPreferredIdentity();
    Q_INVOKABLE void removePreferredIdentity(PreferredIdentity *preferredIdentity);

    Q_INVOKABLE SIPBuddyState::STATUS buddyStatus(const QString &var);

    Q_INVOKABLE bool isConfigured() const;

    Q_INVOKABLE SIPBuddy *getBuddy(const QString &var);

signals:
    void preferredIdentitiesChanged();
    void defaultPreferredIdentityChanged();
    void buddyStateChanged(const QString uri, SIPBuddyState::STATUS status);
    void notConfigured();

private:
    SIPManager(QObject *parent = nullptr);

    void updatePreferredIdentities();

    QList<PreferredIdentity *> m_preferredIdentities;
    QList<PreferredIdentity *> m_enrolledPreferredIdentities;
    QString m_defaultPreferredIdentity;

    std::unique_ptr<AppSettings> m_settings = nullptr;

    SIPMediaConfig *m_mediaConfig = nullptr;
    SIPUserAgentConfig *m_uaConfig = nullptr;
    SIPLogWriter *m_logWriter = nullptr;
    SIPEventLoop *m_ev = nullptr;

    pj::Endpoint m_ep;

    QSet<QString> m_buddyStateQueue;
};

class SIPManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(SIPManager)
    QML_NAMED_ELEMENT(SIPManager)
    QML_SINGLETON

public:
    static SIPManager *create(QQmlEngine *, QJSEngine *) { return &SIPManager::instance(); }

private:
    SIPManagerWrapper() = default;
};
