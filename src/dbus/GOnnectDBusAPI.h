#pragma once
#define API_VERSION 1

#include "ICallState.h"
#include <QObject>
#include <QHash>

class GOnnectDBusAPIAdaptor;

class GOnnectDBusAPI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(unsigned version READ version CONSTANT FINAL)

public:
    explicit GOnnectDBusAPI(QObject *parent = nullptr);
    ~GOnnectDBusAPI();

    void registerCallState(ICallState *state);
    unsigned version() const { return API_VERSION; }

public slots:
    void AddMetaData(const QString &Id, const QString &Data);
    QString Call(const QString &uri);
    QVariantList CallIds();
    void HangupAllCalls();
    void HangupCall(const QString &id);

private:
    void handleCallStateChange(const QString &uuid, ICallState::State state);

    QHash<QString, ICallState::State> m_states;

    GOnnectDBusAPIAdaptor *m_apiAdaptor = nullptr;
    ICallState::States m_oldCallState = ICallState::State::Idle;
};
