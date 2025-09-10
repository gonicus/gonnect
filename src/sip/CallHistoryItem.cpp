#include "CallHistoryItem.h"
#include "CallHistory.h"

CallHistoryItem::CallHistoryItem(const QDateTime &time, const QString &remoteUrl,
                                 const QString &account, const QString &contactId,
                                 bool isSipSubscriptable, qint64 dataBaseId,
                                 quint16 durationSeconds, Types type, CallHistory *parent)
    : QObject{ parent },
      m_dataBaseId{ dataBaseId },
      m_time{ time },
      m_durationSeconds{ durationSeconds },
      m_remoteUrl{ remoteUrl },
      m_account{ account },
      m_contactId{ contactId },
      m_isSipSubscriptable{ isSipSubscriptable },
      m_type{ type }
{
}

CallHistoryItem::CallHistoryItem(const QString &remoteUrl, const QString &account,
                                 const QString &contactId, bool isSipSubscriptable,
                                 CallHistoryItem::Types type, CallHistory *parent)
    : QObject{ parent },
      m_time{ QDateTime::currentDateTime() },
      m_remoteUrl{ remoteUrl },
      m_account{ account },
      m_contactId{ contactId },
      m_isSipSubscriptable{ isSipSubscriptable },
      m_type{ type }
{
}

CallHistory *CallHistoryItem::callHistory() const
{
    return qobject_cast<CallHistory *>(parent());
}

void CallHistoryItem::addFlags(Types flags)
{
    m_type |= flags;
    flushToDatabase();
}

void CallHistoryItem::setContactId(const QString &contactId)
{
    m_contactId = contactId;
    flushToDatabase();
}

void CallHistoryItem::endCall()
{
    const auto now = QDateTime::currentDateTime();
    m_durationSeconds = m_time.secsTo(now);
    flushToDatabase();
}

void CallHistoryItem::flushToDatabase()
{
    CallHistory &history = *callHistory();
    Q_EMIT history.dataChanged(history.indexOfItem(this), this);
    history.writeToDatabase(*this);
}

QDebug operator<<(QDebug debug, const CallHistoryItem &historyItem)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "CallHistoryItem (" << historyItem.time()
                    << ", account: " << historyItem.account()
                    << ", contactId: " << historyItem.contactId()
                    << ", remote: " << historyItem.remoteUrl()
                    << ", duration: " << historyItem.durationSeconds() << " seconds"
                    << ", isSipSubscriptable: " << historyItem.isSipSubscriptable() << ")";
    return debug;
}
