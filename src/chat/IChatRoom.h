#pragma once

#include <QObject>

class IChatRoom : public QObject
{
    Q_OBJECT

public:
    explicit IChatRoom(QObject *parent = nullptr);

    virtual QString id() = 0;
    virtual QString name() = 0;
    virtual qsizetype notificationCount() = 0;

signals:
    void nameChanged(QString name);
    void notificationCountChanged(qsizetype count);
};
