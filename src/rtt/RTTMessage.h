#pragma once

#include <QObject>
#include <QString>

class RTTMessage
{
public:
    explicit RTTMessage(qint64 timestamp, const QString &message, bool isMe, bool isFinished);

    qint64 timestamp() const { return m_timestamp; };
    QString message() const { return m_message; };
    bool isMe() const { return m_isMe; };
    bool isFinished() const { return m_isFinished; };

    void setMessage(const QString &message) { m_message = message; };
    void setIsFinished(bool finished) { m_isFinished = finished; };

private:
    qint64 m_timestamp = 0;
    QString m_message;
    bool m_isMe = false;
    bool m_isFinished = false;
};
