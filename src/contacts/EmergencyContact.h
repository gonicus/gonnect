#pragma once

#include <QObject>

class EmergencyContact : public QObject
{
    Q_OBJECT

public:
    explicit EmergencyContact(quint8 index, QString number, QString displayName,
                              QObject *parent = nullptr);

    quint8 index() const { return m_index; }
    QString number() const { return m_number; }
    QString displayName() const { return m_displayName; }

private:
    quint8 m_index = 0;
    QString m_number;
    QString m_displayName;
};
