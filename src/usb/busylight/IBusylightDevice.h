#pragma once

#include <QObject>
#include <QTimer>
#include <QColor>

#include "hidapi.h"

class IBusylightDevice : public QObject
{
    Q_OBJECT

public:
    enum class SupportedCommands { BusylightOnOff, BusylightColor, StreamlightOnOff };
    Q_ENUM(SupportedCommands)

    virtual QSet<SupportedCommands> supportedCommands() = 0;

    explicit IBusylightDevice(const hid_device_info &deviceInfo, QObject *parent = nullptr);
    virtual ~IBusylightDevice();

    virtual void switchStreamlight(bool on) { Q_UNUSED(on); };

    bool open();
    void close();

    void switchOn(QColor color);
    void switchOff();
    void startBlinking(QColor color);
    void stopBlinking();

    QString path() const { return m_path; }

protected:
    virtual void send(bool on) = 0;
    hid_device *m_device = nullptr;
    QColor m_color;

private:
    QTimer m_blinkTimer;
    QString m_path;
    bool m_isOn = false;
    bool m_isRunning = false;
};
