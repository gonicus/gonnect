#pragma once

#include <QObject>

class MatrixRoom : public QObject
{
    Q_OBJECT

public:
    explicit MatrixRoom(const QString &roomId, const QString name, QObject *parent = nullptr);

    QString roomId() const { return m_roomId; }
    QString name() const { return m_name; }
    quint16 unreadCount() const { return m_unreadCount; }

    void setUnreadCount(quint16 value);

private:
    QString m_roomId;
    QString m_name;
    quint16 m_unreadCount = 0;

signals:
    void unreadCountChanged();
};
