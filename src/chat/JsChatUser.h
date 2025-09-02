#pragma once

#include <QObject>

class JsChatUser : public QObject
{
    Q_OBJECT

public:
    explicit JsChatUser(const QString &userId, const QString &displayName,
                        QObject *parent = nullptr);

    QString userId() const { return m_userId; }
    QString displayName() const { return m_displayName; }

private:
    QString m_userId;
    QString m_displayName;
};
