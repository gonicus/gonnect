#pragma once
#include <QObject>
#include <QTimer>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include <pjsua2.hpp>

#include "ReadOnlyConfdSettings.h"
#include "SIPBuddy.h"

class Toggler : public QObject, public pj::Account
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(QString description READ description CONSTANT FINAL)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged FINAL)
    Q_PROPERTY(unsigned display READ display CONSTANT FINAL)

    QML_ELEMENT
    QML_UNCREATABLE("part of the model")

    Q_DISABLE_COPY(Toggler)

public:
    Toggler(const QString &id, QObject *parent = nullptr);

    enum DISPLAY {
        MENU = 1,
        TRAY = 2,
        STATUS = 4,
        CFG_PHONING = 8,
    };
    Q_ENUM(DISPLAY)

    bool initialize();
    bool isActive() const { return m_buddy ? m_buddy->status() == SIPBuddyState::BUSY : false; }
    bool isBusy() const { return m_busy; }
    void setActive(bool value);

    QString id() const { return m_id; }
    QString name() const { return m_label; }
    QString description() const { return m_description; }
    unsigned display() const { return m_display; }

    ~Toggler();

Q_SIGNALS:
    void activeChanged();
    void busyChanged();

private:
    ReadOnlyConfdSettings m_settings;
    QTimer m_timeoutTimer;

    QString m_id;
    QString m_description;
    QString m_label;
    QString m_toggle;
    QString m_subscribe;

    SIPBuddy *m_buddy = nullptr;
    SIPAccount *m_account = nullptr;

    unsigned m_display = 0;

    bool m_busy = false;
};
