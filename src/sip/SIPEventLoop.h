#pragma once
#include <QTimer>
#include <QObject>

class SIPEventLoop : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SIPEventLoop)

public:
    SIPEventLoop(QObject *parent = nullptr);
    ~SIPEventLoop();

private:
    QTimer ticker;
};
