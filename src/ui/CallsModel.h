#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>
#include <pjsip/sip_msg.h>
#include "PhoneNumberUtil.h"
#include "SIPCall.h"

class CallsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int unfinishedCount READ unfinishedCount NOTIFY unfinishedCountChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

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
        bool hasMetadata = false;
        QDateTime established;
        ContactInfo contactInfo;
        pjsip_status_code statusCode = PJSIP_SC_NULL;
        SIPCallManager::QualityLevel qualityLevel = SIPCallManager::QualityLevel::Low;
        SIPCallManager::SecurityLevel securityLevel = SIPCallManager::SecurityLevel::Low;
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
        HasMetadata,
        HasAvatar,
        AvatarPath,
        QualityLevel,
        SecurityLevel
    };

    explicit CallsModel(QObject *parent = nullptr);
    ~CallsModel();

    qsizetype count() const { return m_calls.size(); }
    int unfinishedCount() const;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void countChanged();
    void unfinishedCountChanged();

private Q_SLOTS:
    void updateCalls();

private:
    QList<CallInfo *> m_calls;
    QHash<int, CallInfo *> m_callsHash;
};
