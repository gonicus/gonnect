#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

class RTTMessage
{
    Q_GADGET

public:
    explicit RTTMessage(const QDateTime &timestamp, const QString &sender, const QString &message,
                        bool isMe, bool isFinished);

    QDateTime timestamp() const { return m_timestamp; };
    QString sender() const { return m_sender; };
    QString message() const { return m_message; };
    bool isMe() const { return m_isMe; };
    bool isFinished() const { return m_isFinished; };

    void setMessage(const QString &message) { m_message = message; };
    void setIsFinished(bool finished) { m_isFinished = finished; };

private:
    QDateTime m_timestamp;
    QString m_sender;
    QString m_message;
    bool m_isMe;
    bool m_isFinished = false;
};
