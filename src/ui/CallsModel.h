#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>
#include <pjsip/sip_msg.h>
#include "PhoneNumberUtil.h"

class SIPCall;

class CallsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    struct CallInfo
    {
        int callId = 0;
        QString accountId;
        QString remoteUri;
        bool isIncoming = false;
        bool isHolding = false;
        bool isBlocked = false;
        bool isEstablished = false;
        bool isFinished = false;
        bool hasCapabilityJitsi = false;
        qreal incomingAudioLevel = 0.0;
        QDateTime established;
        ContactInfo contactInfo;
        pjsip_status_code statusCode = PJSIP_SC_NULL;
    };

    enum class Roles {
        CallId = Qt::UserRole + 1,
        AccountId,
        RemoteUri,
        PhoneNumber,
        ContactName,
        IsIncoming,
        City,
        Country,
        Company,
        IsEstablished,
        EstablishedTime,
        IsHolding,
        IsBlocked,
        StatusCode,
        IsFinished,
        HasCapabilityJitsi,
        HasIncomingAudioLevel,
        HasAvatar,
        AvatarPath
    };

    explicit CallsModel(QObject *parent = nullptr);
    virtual ~CallsModel();

    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

private slots:
    void updateCalls();

private:
    QList<CallInfo *> m_calls;
    QHash<int, CallInfo *> m_callsHash;
};
