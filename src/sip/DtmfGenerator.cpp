#include "DtmfGenerator.h"
#include <QLoggingCategory>
#include "AudioManager.h"

Q_LOGGING_CATEGORY(lcDtmfGenerator, "gonnect.sip.dtmfgenerator")

DtmfGenerator::DtmfGenerator(QObject *parent)
    : QObject{ parent }, m_mediaSink(AudioManager::instance().getPlaybackDevMedia())
{

    try {
        m_toneGen.createToneGenerator();
        m_toneGen.startTransmit(m_mediaSink);
    } catch (const pj::Error &err) {
        qCCritical(lcDtmfGenerator) << "failed to initialize DTMF tone generator:" << err.info();
    }
}

void DtmfGenerator::playDtmf(const QChar &key)
{
    const auto lower = key.toLower();

    if (!isValid(lower)) {
        qCWarning(lcDtmfGenerator) << "Ignoring unknown DTMF char" << lower;
        return;
    }

    pj::ToneDigit digitDesc;
    digitDesc.digit = lower.toLatin1();
    digitDesc.on_msec = PJSUA_CALL_SEND_DTMF_DURATION_DEFAULT;
    digitDesc.off_msec = 0;

    try {
        m_toneGen.playDigits({ digitDesc });
    } catch (const pj::Error &err) {
        qCWarning(lcDtmfGenerator) << "failed to play DTMF tone" << lower << ":" << err.info();
    }
}

bool DtmfGenerator::isValid(const QChar &key)
{
    const static QSet<QChar> allowedChars = { '0', '1', '2', '3', '4', '5', '6', '7',
                                              '8', '9', '#', '*', 'a', 'b', 'c', 'd' };
    return allowedChars.contains(key.toLower());
}
