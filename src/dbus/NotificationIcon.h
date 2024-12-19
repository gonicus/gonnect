#pragma once

#include <QFileInfo>
#include <QDBusArgument>

class NotificationIcon
{

public:
    NotificationIcon() = default;
    ~NotificationIcon() = default;

    explicit NotificationIcon(const QString &fileUri, const QString &emblem = "",
                              bool rounded = true);

    QByteArray data() const { return m_data; }
    QString imageName() const { return m_imageName; }

private:
    QString m_imageName;
    QByteArray m_data;
};

Q_DECLARE_METATYPE(NotificationIcon)

QDBusArgument &operator<<(QDBusArgument &argument, const NotificationIcon &ni);
const QDBusArgument &operator>>(const QDBusArgument &argument, NotificationIcon &ni);
