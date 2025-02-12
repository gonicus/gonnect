#include <QLoggingCategory>
#include <QTimer>
#include <pjsua2.hpp>

#include "SIPMediaConfig.h"
#include "SIPManager.h"
#include "ReadOnlyConfdSettings.h"

Q_LOGGING_CATEGORY(lcSIPMediaConfig, "gonnect.sip.config.media")

using std::chrono::seconds;
using namespace std::chrono_literals;

SIPMediaConfig::SIPMediaConfig(SIPManager *parent) : QObject(parent) { }

void SIPMediaConfig::applyConfig(pj::EpConfig &epConfig)
{
    ReadOnlyConfdSettings settings;
    bool ok = false;

    bool noVad = settings.value("media/noVad", false).toBool();
    epConfig.medConfig.noVad = noVad;

    bool useSoftwareClock = settings.value("media/softwareClock", true).toBool();
    epConfig.medConfig.sndUseSwClock = useSoftwareClock;

    unsigned clockRate = settings.value("media/clockRate", PJSUA_DEFAULT_CLOCK_RATE).toUInt(&ok);
    if (ok) {
        epConfig.medConfig.clockRate = clockRate;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for clockRate, using default";
        epConfig.medConfig.clockRate = PJSUA_DEFAULT_CLOCK_RATE;
    }

    unsigned sndClockRate = settings.value("media/sndClockRate", clockRate).toUInt(&ok);
    if (ok) {
        epConfig.medConfig.sndClockRate = sndClockRate;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for sndClockRate, using default";
        epConfig.medConfig.sndClockRate = epConfig.medConfig.clockRate;
    }

    unsigned sndRecLatency =
            settings.value("media/sndRecLatency", PJMEDIA_SND_DEFAULT_REC_LATENCY).toUInt(&ok);
    if (ok) {
        epConfig.medConfig.sndRecLatency = sndRecLatency;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for sndRecLatency, using default";
        epConfig.medConfig.sndRecLatency = PJMEDIA_SND_DEFAULT_REC_LATENCY;
    }

    unsigned sndPlayLatency =
            settings.value("media/sndPlayLatency", PJMEDIA_SND_DEFAULT_PLAY_LATENCY).toUInt(&ok);
    if (ok) {
        epConfig.medConfig.sndPlayLatency = sndPlayLatency;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for sndPlayLatency, using default";
        epConfig.medConfig.sndPlayLatency = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;
    }

    unsigned quality = settings.value("media/quality", PJSUA_DEFAULT_CODEC_QUALITY).toUInt(&ok);
    if (ok && quality < 11) {
        epConfig.medConfig.quality = quality;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for quality, using default";
        epConfig.medConfig.quality = PJSUA_DEFAULT_CODEC_QUALITY;
    }

    unsigned audioFramePtime =
            settings.value("media/audioFramePtime", PJSUA_DEFAULT_AUDIO_FRAME_PTIME).toUInt(&ok);
    if (ok) {
        epConfig.medConfig.audioFramePtime = audioFramePtime;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for audioFramePtime, using default";
        epConfig.medConfig.audioFramePtime = PJSUA_DEFAULT_AUDIO_FRAME_PTIME;
    }

    int jbInit = settings.value("media/jbInit", -1).toInt(&ok);
    if (ok) {
        epConfig.medConfig.jbInit = jbInit;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for jbInit, using default";
        epConfig.medConfig.jbInit = -1;
    }

    int jbMinPre = settings.value("media/jbMinPre", -1).toInt(&ok);
    if (ok) {
        epConfig.medConfig.jbMinPre = jbMinPre;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for jbMinPre, using default";
        epConfig.medConfig.jbMinPre = -1;
    }

    int jbMaxPre = settings.value("media/jbMaxPre", -1).toInt(&ok);
    if (ok) {
        epConfig.medConfig.jbMaxPre = jbMaxPre;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for jbMaxPre, using default";
        epConfig.medConfig.jbMaxPre = -1;
    }

    int jbMax = settings.value("media/jbMax", -1).toInt(&ok);
    if (ok) {
        epConfig.medConfig.jbMax = jbMax;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for jbMax, using default";
        epConfig.medConfig.jbMax = -1;
    }

    // TODO:
    //  jbDiscardAlgo   default PJMEDIA_JB_DISCARD_PROGRESSIVE, PJMEDIA_JB_DISCARD_NONE,
    //  PJMEDIA_JB_DISCARD_STATIC ecOptions       -> check enum
    //  https://docs.pjsip.org/en/latest/api/generated/pjmedia/group/group__PJMEDIA__Echo__Cancel.html#group__PJMEDIA__Echo__Cancel_1gaa92df3d6726a21598e25bf5d4a23897e

    unsigned ecTailLen = settings.value("media/ecTailLen", PJSUA_DEFAULT_EC_TAIL_LEN).toUInt(&ok);
    if (ok) {
        epConfig.medConfig.ecTailLen = ecTailLen;
    } else {
        qCWarning(lcSIPMediaConfig) << "invalid value for ecTailLen, using default";
        epConfig.medConfig.ecTailLen = PJSUA_DEFAULT_EC_TAIL_LEN;
    }
}
