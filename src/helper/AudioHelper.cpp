#include "AudioHelper.h"
#include "SIPAudioDevice.h"

#include <QMediaDevices>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcAudioHelper, "gonnect.helper.AudioHelper")

QAudioOutput *AudioHelper::createAudioOutput(const QString &ringerHash, QObject *parent)
{
    if (!ringerHash.isEmpty()) {
        const QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();
        for (const QAudioDevice &device : audioDevices) {
            auto calculatedHash = SIPAudioDevice::makeHash(device.description(), false);
            if (calculatedHash == ringerHash) {
                return new QAudioOutput(device, parent);
            }
        }
        qCCritical(lcAudioHelper) << "unknown ringing device in config - using default";
    }
    return new QAudioOutput(QMediaDevices::defaultAudioOutput(), parent);
}

qreal AudioHelper::calculateVolume(qreal customVolume, qreal configVolumePercent)
{
    return (0.0 <= customVolume && customVolume <= 1.0) ? customVolume
                                                        : configVolumePercent / 100.0;
}

AudioHelper::AudioHelper() { }
