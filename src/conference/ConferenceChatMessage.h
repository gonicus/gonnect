#pragma once

#include <QObject>
#include <QDateTime>

class ConferenceChatMessage : public QObject
{
    Q_OBJECT

public:
    explicit ConferenceChatMessage(const QString &fromId, const QString &nickName,
                                   const QString &message, const QDateTime &timestamp,
                                   bool isPrivateMessage, bool isSystemMessage,
                                   QObject *parent = nullptr);

    QString fromId() const { return m_fromId; }
    QString nickName() const { return m_nickName; }
    QString message() const { return m_message; }
    QDateTime timestamp() const { return m_timestamp; }
    bool isPrivateMessage() const { return m_isPrivateMessage; }
    bool isSystemMessage() const { return m_isSystemMessage; }

private:
    QString m_fromId;
    QString m_nickName;
    QString m_message;
    QDateTime m_timestamp;
    bool m_isPrivateMessage = false;
    bool m_isSystemMessage = false;
};
