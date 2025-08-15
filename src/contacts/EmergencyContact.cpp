#include "EmergencyContact.h"

EmergencyContact::EmergencyContact(quint8 index, QString number, QString displayName,
                                   QObject *parent)
    : QObject{ parent }, m_index{ index }, m_number{ number }, m_displayName{ displayName }
{
}
