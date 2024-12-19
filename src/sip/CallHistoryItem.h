#pragma once

#include <QObject>
#include <QDateTime>
#include <qqmlintegration.h>

class CallHistory;

class CallHistoryItem : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Unused in Qml; just registered for enum access")
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    enum class Type { Incoming, Outgoing, IncomingBlocked };
    Q_ENUM(Type)

    explicit CallHistoryItem(const QDateTime &time, const QString &remoteUrl,
                             const QString &account, const QString &contactId,
                             bool isSipSubscriptable, qint64 dataBaseId, quint16 durationSeconds,
                             CallHistoryItem::Type type, CallHistory *parent);

    explicit CallHistoryItem(const QString &remoteUrl, const QString &account,
                             const QString &contactId, bool isSipSubscriptable,
                             CallHistoryItem::Type type, CallHistory *parent);

    CallHistory *callHistory() const;

    void setDataBaseId(qint64 id) { m_dataBaseId = id; }
    qint64 dataBaseId() const { return m_dataBaseId; }
    QDateTime time() const { return m_time; }
    QString remoteUrl() const { return m_remoteUrl; }
    QString account() const { return m_account; }
    QString contactId() const { return m_contactId; }
    quint16 durationSeconds() const { return m_durationSeconds; }
    bool isSipSubscriptable() const { return m_isSipSubscriptable; }
    CallHistoryItem::Type type() const { return m_type; }

    void setContactId(const QString &contactId);

    void endCall();

private:
    void flushToDatabase();

    qint64 m_dataBaseId = -1;
    QDateTime m_time;
    quint16 m_durationSeconds = 0;
    QString m_remoteUrl;
    QString m_account;
    QString m_contactId;
    bool m_isSipSubscriptable = false;
    CallHistoryItem::Type m_type = Type::Incoming;
};

QDebug operator<<(QDebug debug, const CallHistoryItem &historyItem);
