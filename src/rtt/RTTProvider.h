#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

#include "RTTModel.h"
#include "ICallState.h"
#include "SIPCall.h"

class RTTProvider : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(RTTProvider)

    Q_PROPERTY(RTTModel *model READ model CONSTANT)
    Q_PROPERTY(bool hasMessages READ hasMessages NOTIFY hasMessagesChanged FINAL)
    Q_PROPERTY(bool isEstablishedCall READ isEstablishedCall NOTIFY isEstablishedCallChanged FINAL)
    Q_PROPERTY(bool isRttCall READ isRttCall NOTIFY isRttCallChanged FINAL)

public:
    Q_REQUIRED_RESULT static RTTProvider &instance()
    {
        static RTTProvider *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new RTTProvider();
        }

        return *_instance;
    }

    ~RTTProvider();

    RTTModel *model() const { return m_model; }

    /// Determine QML visibility based on messages being present
    bool hasMessages();

    /// Determine QML visibility based on call state
    bool isEstablishedCall();

    /// Determine QML visibility based on RTT enablement
    bool isRttCall();

    /// These methods wrap the SIPCall RTT functionality
    Q_INVOKABLE void rttSend(const QString &text);
    Q_INVOKABLE void rttSendLineSeperator();
    Q_INVOKABLE void rttSendCRLF();
    Q_INVOKABLE void rttSendBackspace();
    Q_INVOKABLE void rttSendBell();

Q_SIGNALS:
    void hasMessagesChanged();
    void isEstablishedCallChanged();
    void isRttCallChanged();

private:
    RTTProvider(QObject *parent = nullptr);

    RTTModel *m_model = nullptr;

    ICallState *m_state = nullptr;
    SIPCall *m_call = nullptr;

    QMetaObject::Connection m_establishedCall;
    QMetaObject::Connection m_rttCall;
    QMetaObject::Connection m_rttBubbleChanged;
    QMetaObject::Connection m_rttBubbleCommitted;

    bool m_newMessage = false;
};

class RTTProviderWrapper
{
    Q_GADGET
    QML_FOREIGN(RTTProvider)
    QML_NAMED_ELEMENT(RTTProvider)
    QML_SINGLETON

public:
    static RTTProvider *create(QQmlEngine *, QJSEngine *) { return &RTTProvider::instance(); }

private:
    RTTProviderWrapper() = default;
};
