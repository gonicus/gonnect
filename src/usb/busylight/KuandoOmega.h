#pragma once

#include <QTimer>

#include "IBusylightDevice.h"
#include "hidapi.h"

class KuandoOmega : public IBusylightDevice
{
    Q_OBJECT

public:
    explicit KuandoOmega(const hid_device_info &deviceInfo, QObject *parent = nullptr);


protected:
    virtual void send(bool on) override;

private:
    void sendKeepAlive() const;

    QTimer m_keepAliveTimer;
};
