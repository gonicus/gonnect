#pragma once

#include <QAudioOutput>
#include <QString>
#include <QLoggingCategory>

class AudioHelper
{

public:
    static QAudioOutput *createAudioOutput(const QString &ringerHash, QObject *parent);
    static qreal calculateVolume(qreal customVolume, qreal configVolumePercent);

private:
    AudioHelper();
};
