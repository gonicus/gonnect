#pragma once

#include <QObject>
#include <QTimer>
#include <QColor>

#include "hidapi.h"

class IBusylightDevice : public QObject
{
    Q_OBJECT

public:
    explicit IBusylightDevice(const hid_device_info &deviceInfo, QObject *parent = nullptr);
    virtual ~IBusylightDevice();

    bool open();
    void close();

    void switchOn(QColor color);
    void switchOff();
    void startBlinking(QColor color);
    void stopBlinking();

protected:
    virtual void send(bool on) = 0;
    hid_device *m_device = nullptr;
    QColor m_color;

private:
    QTimer m_blinkTimer;
    QString m_path;
    bool m_isOn = false;
};
