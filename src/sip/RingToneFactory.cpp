#include <QLoggingCategory>

#include "RingToneFactory.h"
#include "RingTone.h"
#include "ReadOnlyConfdSettings.h"

Q_LOGGING_CATEGORY(lcRingToneFactory, "gonnect.app.ringtoneFactory")

RingToneFactory::RingToneFactory(QObject *parent) : QObject{ parent } { }

void RingToneFactory::loadConfig()
{

    ReadOnlyConfdSettings settings;

    QString tonedef = settings.value("dialtones/ring", tr("ringTone")).toString();
    if (tonedef == "ringTone") {
        tonedef = "425,0,1000,4000,0";
    }
    m_ringingTone = createRingToneObjectFromConfigString(tonedef);

    tonedef = settings.value("dialtones/busy", tr("busyTone")).toString();
    if (tonedef == "busyTone") {
        tonedef = "425,0,480,480,0";
    }
    m_busyTone = createRingToneObjectFromConfigString(tonedef);

    tonedef = settings.value("dialtones/congestion", tr("congestionTone")).toString();
    if (tonedef == "congestionTone") {
        tonedef = "425,0,240,240,0";
    }
    m_congestionTone = createRingToneObjectFromConfigString(tonedef);

    tonedef = settings.value("dialtones/zip", tr("zip")).toString();
    if (tonedef == "zip") {
        tonedef = "425,0,200,200,200,1000,200,200,200,1000,200,200,200,5000,4";
    }
    m_zipTone = createRingToneObjectFromConfigString(tonedef);

    tonedef = settings.value("dialtones/end", tr("endTone")).toString();
    if (tonedef == "endTone") {
        tonedef = "425,0,200,200,200,200,200,200,-1";
    }
    m_endTone = createRingToneObjectFromConfigString(tonedef);

    m_configLoaded = true;
}

RingTone *RingToneFactory::createRingToneObjectFromConfigString(const QString &configString)
{

    const auto split = configString.split(',');

    if (split.size() < 4) {
        qCFatal(lcRingToneFactory, "Ring tone config string is invalid");
    }

    bool ok;
    const quint16 freq1 = split.at(0).toUInt(&ok, 10);
    if (!ok) {
        qCFatal(lcRingToneFactory, "freq1 cannot be parsed to a frequency");
    }
    const quint16 freq2 = split.at(1).toUInt(&ok, 10);
    if (!ok) {
        qCFatal(lcRingToneFactory, "freq2 cannot be parsed to a frequency");
    }

    QList<QPair<quint16, quint16>> intervals;
    for (int i = 2; i < split.size() - 1; i = i + 2) {
        quint16 int1 = split.at(i).toUInt(&ok, 10);
        if (!ok) {
            qCFatal(lcRingToneFactory, "Number could not be parsed as time interval");
        }
        quint16 int2 = split.at(i + 1).toUInt(&ok, 10);
        if (!ok) {
            qCFatal(lcRingToneFactory, "Number could not be parsed as time interval");
        }
        intervals.push_back({ int1, int2 });
    }

    qint8 loopIndex = -1;
    if (split.size() % 2) {

        loopIndex = split.last().toInt(&ok, 10);
        if (!ok) {
            qCFatal(lcRingToneFactory, "Loop index could not be parsed as integer");
        }
    }

    return new RingTone(freq1, freq2, intervals, loopIndex, this);
}

RingTone *RingToneFactory::ringingTone()
{
    if (!m_configLoaded) {
        loadConfig();
    }

    return m_ringingTone;
}

RingTone *RingToneFactory::busyTone()
{
    if (!m_configLoaded) {
        loadConfig();
    }

    return m_busyTone;
}

RingTone *RingToneFactory::congestionTone()
{
    if (!m_configLoaded) {
        loadConfig();
    }

    return m_congestionTone;
}

RingTone *RingToneFactory::zipTone()
{
    if (!m_configLoaded) {
        loadConfig();
    }

    return m_zipTone;
}

RingTone *RingToneFactory::endTone()
{
    if (!m_configLoaded) {
        loadConfig();
    }

    return m_endTone;
}
