#include "DtmfGenerator.h"
#include <QLoggingCategory>
#include "SIPAudioManager.h"

Q_LOGGING_CATEGORY(lcDtmfGenerator, "gonnect.sip.dtmfgenerator")

DtmfGenerator::DtmfGenerator(QObject *parent)
    : QObject{ parent }, m_mediaSink(SIPAudioManager::instance().getPlaybackDevMedia())
{

    m_toneGen.createToneGenerator();
    m_toneGen.startTransmit(m_mediaSink);
}

void DtmfGenerator::playDtmf(const QChar &key)
{
    const auto lower = key.toLower();
    const static QSet<QChar> allowedChars = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
                                              '9', '0', '#', '*', 'a', 'b', 'c', 'd' };

    if (!allowedChars.contains(lower)) {
        qCWarning(lcDtmfGenerator) << "Ignoring unknown DTMF char" << lower;
        return;
    }

    pj::ToneDigit digitDesc;
    digitDesc.digit = lower.toLatin1();
    digitDesc.on_msec = PJSUA_CALL_SEND_DTMF_DURATION_DEFAULT;
    digitDesc.off_msec = 0;

    m_toneGen.playDigits({ digitDesc });
}
