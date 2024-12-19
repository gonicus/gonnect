#include <QLoggingCategory>
#include <QCryptographicHash>
#include <QMediaDevices>
#include "SIPAudioDevice.h"

Q_LOGGING_CATEGORY(lcSIPAudioDevice, "gonnect.sip.audio")

SIPAudioDevice::SIPAudioDevice(const QString &name, bool input, bool defaultDevice, QObject *parent)
    : QObject(parent), m_isInput(input), m_default(defaultDevice), m_name(name)
{
    m_hash = SIPAudioDevice::makeHash(m_name, m_isInput);
}

SIPAudioDevice::~SIPAudioDevice() { }

QString SIPAudioDevice::makeHash(const QString &name, bool input)
{
    auto id = QString("%1|%2").arg(name, input ? "IN" : "OUT");
    return QString(QCryptographicHash::hash((id.toLocal8Bit()), QCryptographicHash::Md5).toHex());
}
